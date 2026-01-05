#pragma once

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>

class LogManager{
public:
    static void logError(const std::string& context, bool includeErrno = true);   
};

#define LOG_ERROR(what) \
    LogManager::logError(std::string(__func__) + " -> " + what)

