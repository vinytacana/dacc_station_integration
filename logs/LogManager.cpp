#include "LogManager.hpp"

void LogManager::logError(const std::string& context, bool includeErrno){
    std::cerr << "[ERROR] " << context;

    if (includeErrno) {
        std::cerr << " | errno " << errno << ": " << std::strerror(errno);
    }
    std::cerr << std::endl;
}

