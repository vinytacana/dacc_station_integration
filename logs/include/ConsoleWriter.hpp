#pragma once

#include <iostream>
#include "ILogWriter.hpp"

class ConsoleWriter : public ILogWriter
{
private:
    
public:
    void write(const std::string& message){
        std::cout << message;
        std::cout.flush();
    }
};
