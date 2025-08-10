#include "LogManager.hpp"

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>

static const SockAddr logger_socket{};

int main(){
    
    int stream_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); 
    
    if (stream_socket_fd < 0) {
        LOG_ERROR("socket()"); return 1;
    }

    unlink(SOCKET_PATH);  

    //bind() espera um ponteiro para struct sockaddr
    if (bind(stream_socket_fd,
        (struct sockaddr*)&logger_socket.addr,
        sizeof(logger_socket.addr)) < 0) {
            
        LOG_ERROR("bind()"); return 1;
    }
    
    if (listen(stream_socket_fd, 2) < 0) {
        LOG_ERROR("listen()"); return 1;
    }

    std::ofstream log_file("system.log.txt", std::ios::app);
    
    std::cout << "LogServer waiting conections...\n";

    if(not log_file){
        LOG_ERROR("log_file"); return 1;
    }

    while (true) {
        int client_fd = accept(stream_socket_fd, nullptr, nullptr);
        
        if (client_fd < 0) {
            LOG_ERROR("accept()");
            continue;
        }

        // reads until client closes
        char buf[1024];
        ssize_t n;
        while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
            log_file.write(buf, n);
            log_file.flush();
        }
        close(client_fd);
    }    

    close(stream_socket_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}