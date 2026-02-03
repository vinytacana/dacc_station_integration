#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <cstring>

class UnixSocketUtils {
public:
    /**
     * Prepara a estrutura sockaddr_un com segurança.
     * @param addr Referência para a estrutura a ser preenchida.
     * @param path Caminho do socket.
     */
    static void prepareAddress(sockaddr_un& addr, const std::string& path) {
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    }
};
