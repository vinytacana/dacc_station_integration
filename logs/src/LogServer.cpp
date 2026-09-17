#include "LogManager.hpp"
#include "ipc/LocalSocket.hpp"
#include "LogProtocol.hpp"
#include <unordered_set>
#include "json.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <csignal>
#include <atomic>
#include <filesystem>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

volatile sig_atomic_t shutdown_flag = 0;

void signal_handler(int signal){
    if(signal == SIGINT || signal == SIGTERM){
        shutdown_flag = true;
    }
}

int main(){
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int stream_socket_fd = -1;
    int epoll_fd = -1;
    std::string socket_path;
    std::unique_ptr<ipc::LocalSocketServer> server;
    std::unordered_set<int> clients;
    LogProtocol protocol;
    std::shared_ptr<spdlog::logger> server_logger;

    try {
        std::string config_path = "config.json";

        if (!fs::exists(config_path)) config_path = "../config.json";
        if (!fs::exists(config_path)) config_path = "process-manager/config.json";

        std::ifstream config_file(config_path);

        if (not config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }

        nlohmann::json config = nlohmann::json::parse(config_file);

        socket_path = ipc::logSocketPath();
        const std::string output_type = config["output"]["type"];

        if (output_type == "file") {
            std::string filepath = config["output"]["filepath"];
            
            fs::path p(filepath);
            if (p.has_parent_path() && !fs::exists(p.parent_path())) {
                fs::create_directories(p.parent_path());
            }

            auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filepath, 1024 * 1024 * 5, 3
            );
            
            server_logger = std::make_shared<spdlog::logger>("server_logger", rotating_sink);
            
        } else if (output_type == "console") {
            server_logger = spdlog::stdout_color_mt("server_logger");
        } else {
            throw std::runtime_error("Invalid output type in config: " + output_type);
        }

        server_logger->set_pattern("%v");
        server_logger->flush_on(spdlog::level::info);

        server = std::make_unique<ipc::LocalSocketServer>(socket_path);
        stream_socket_fd = server->fd();

        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            std::cerr << "[LogServer] CRITICAL: epoll_create1() failed" << std::endl;
            throw std::runtime_error("Failed to create epoll instance");
        }

        epoll_event event{};
        event.events = EPOLLIN; 
        event.data.fd = stream_socket_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream_socket_fd, &event) < 0) {
            std::cerr << "[LogServer] CRITICAL: epoll_ctl() failed" << std::endl;
            throw std::runtime_error("Failed to add server socket to epoll");
        }
        
        while (!shutdown_flag) {
            epoll_event events[10];
            int n_events = epoll_wait(epoll_fd, events, 10, 1000);

            if (n_events < 0) {
                if (errno == EINTR) continue;
                std::cerr << "[LogServer] epoll_wait() failed" << std::endl;
                break;
            }

            for (int i = 0; i < n_events; ++i) {
                if (events[i].data.fd == stream_socket_fd) {
                    int client_fd = accept4(stream_socket_fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (client_fd < 0) {
                        continue;
                    }
                    const auto admission = protocol.admit(client_fd, geteuid());
                    if (admission != LogAdmission::Accepted) {
                        server_logger->warn("Rejected log client: {}", static_cast<int>(admission));
                        close(client_fd);
                        continue;
                    }

                    epoll_event ev_client{};
                    ev_client.events = EPOLLIN;
                    ev_client.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev_client) != 0) {
                        protocol.remove(client_fd);
                        close(client_fd);
                        continue;
                    }
                    clients.insert(client_fd);
                } else {
                    int client_fd = events[i].data.fd;
                    char buf[4096]; 
                    ssize_t n;
                    bool closed = false;

                    for (int batch = 0; batch < 16 && !shutdown_flag; ++batch) {
                        n = read(client_fd, buf, sizeof(buf));
                        if (n > 0) {
                            auto result = protocol.feed(client_fd, buf, static_cast<std::size_t>(n));
                            for (const auto& message : result.messages) server_logger->info(message);
                            if (result.overflowed) { closed = true; break; }
                        } else if (n == 0) {
                            closed = true;
                            break;
                        } else {
                            if (errno == EINTR) continue;
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            } else {
                                closed = true;
                                break;
                            }
                        }
                    }

                    if (closed) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                        close(client_fd);
                        protocol.remove(client_fd);
                        clients.erase(client_fd);
                    }
                }
            }
        }    
    
    } catch (const std::exception& e) {
        std::cerr << "[LogServer] Exception: " << e.what() << std::endl;
        
        if (epoll_fd >= 0) close(epoll_fd);
        for (int client : clients) close(client);
        server.reset();
        return 1;
    }
    
    if (epoll_fd >= 0) close(epoll_fd);
    for (int client : clients) close(client);
    server.reset();

    return 0;
}
