#pragma once

#include <string>
#include <memory>
#include "ILogger.hpp"

class LogManager {
public:
    static LogManager& getInstance();

    void initialize(const std::string& appName, const std::string& socketPath);
    std::shared_ptr<ILogger> getLogger();

    // Legacy static method (can be deprecated or kept for convenience)
    static void logError(const std::string& context, bool includeErrno = true);

private:
    LogManager() = default;
    std::shared_ptr<ILogger> logger_;
    std::string appName_;
};

#define LOG_ERROR(what) \
    LogManager::getInstance().getLogger()->error(std::string(__func__) + " -> " + what)

#define LOG_INFO(what) \
    LogManager::getInstance().getLogger()->info(what)

#define LOG_WARNING(what) \
    LogManager::getInstance().getLogger()->warning(what)

#define LOG_DEBUG(what) \
    LogManager::getInstance().getLogger()->debug(what)

