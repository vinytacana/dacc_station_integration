#pragma once 
#include <string>

class ILogger{
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;

    void debug(const std::string& msg) { log("[DEBUG] " + msg); }
    void info(const std::string& msg) { log("[INFO] " + msg); }
    void warning(const std::string& msg) { log("[WARNING] " + msg); }
    void error(const std::string& msg) { log("[ERROR] " + msg); }
    void fatal(const std::string& msg) { log("[FATAL] " + msg); }
};