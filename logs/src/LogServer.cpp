#include "LogManager.hpp"
#include "ILogWriter.hpp"
#include "FileWriter.hpp"
#include "ConsoleWriter.hpp"
#include "json.hpp"

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <csignal>
#include <atomic>

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <stdexcept>

std::atomic<bool> shutdown_flag{false};

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

    try {
        std::ifstream config_file("../config.json");

        if (not config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }

        nlohmann::json config = nlohmann::json::parse(config_file);

        socket_path = config["server"]["socket_path"];
        const std::string output_type = config["output"]["type"];

        std::unique_ptr<ILogWriter> writer;
        if (output_type == "file") {
            const std::string filepath = config["output"]["filepath"];
            writer = std::make_unique<FileWriter>(filepath);

            std::cout << "LogServer: Configured to write to file -> " << filepath << std::endl;
        } else if (output_type == "console") {
            writer = std::make_unique<ConsoleWriter>();

            std::cout << "LogServer: Configured to write to console." << std::endl;
        } else {
            throw std::runtime_error("Invalid output type in config: " + output_type);
        }

        sockaddr_un addr {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

        stream_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); 
        if (stream_socket_fd < 0) {
            LOG_ERROR("socket()"); return 1;
        }

        unlink(socket_path.c_str());  

        if (bind(stream_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            LOG_ERROR("bind()");
            close(stream_socket_fd);
            return 1;
        }
        
        if (listen(stream_socket_fd, 5) < 0) {
            LOG_ERROR("listen()");
            close(stream_socket_fd);
            return 1;
        }

        // Configurar epoll para esperar por conexões de forma não-bloqueante
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            LOG_ERROR("epoll_create1()");
            throw std::runtime_error("Failed to create epoll instance");
        }

        epoll_event event{};
        event.events = EPOLLIN; 
        event.data.fd = stream_socket_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stream_socket_fd, &event) < 0) {
            LOG_ERROR("epoll_ctl()");
            throw std::runtime_error("Failed to add server socket to epoll");
        }
        
        std::cout << "LogServer: Waiting for connections on " << socket_path << "..." << std::endl;

        while (!shutdown_flag) {
            epoll_event events[1];
            // Esperar por eventos com um timeout de 1 segundo
            int n_events = epoll_wait(epoll_fd, events, 1, 1000);

            if (n_events < 0) {
                // Se interrompido por um sinal, o loop continuará e verificará a flag
                if (errno == EINTR) continue;
                LOG_ERROR("epoll_wait()");
                break;
            }

            if (n_events == 0) {
                // Timeout, nenhuma conexão. O loop continua e verifica a flag.
                continue;
            }

            // Se houver um evento, é uma nova conexão
            int client_fd = accept(stream_socket_fd, nullptr, nullptr);
            if (client_fd < 0) {
                LOG_ERROR("accept()");
                continue;
            }

            char buf[1024];
            ssize_t n;
            while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
                writer->write(std::string(buf, n));
            }
            close(client_fd);
        }    
    
    } catch (const std::exception& e) {
        LOG_ERROR(e.what());
        
        if (epoll_fd >= 0) close(epoll_fd);
        if (stream_socket_fd >= 0) close(stream_socket_fd);
        if (!socket_path.empty()) unlink(socket_path.c_str());
        return 1;
    }
    

    std::cout << "\nLogServer: Shutdown signal received. Cleaning up..." << std::endl;
    if (epoll_fd >= 0) close(epoll_fd);
    if (stream_socket_fd >= 0) close(stream_socket_fd);
    if (!socket_path.empty()) unlink(socket_path.c_str());
    std::cout << "LogServer: Cleanup complete. Exiting." << std::endl;

    return 0;
}