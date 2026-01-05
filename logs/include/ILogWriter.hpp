#pragma once
#include <string>

class ILogWriter
{
public:
    virtual ~ILogWriter() = default;
    virtual void write(const std::string& message) = 0;
};