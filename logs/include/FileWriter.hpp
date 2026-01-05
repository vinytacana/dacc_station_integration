#pragma once

#include <fstream>
#include <stdexcept>

#include "LogManager.hpp"
#include "ILogWriter.hpp"
 
class FileWriter : public ILogWriter
{
private:
    std::ofstream file_;
public:
    
    explicit FileWriter(const std::string& filename) {
        file_.open(filename, std::ios::out | std::ios::app);

        if(not file_.is_open()){
            LOG_ERROR("Failed to open log file: " + filename);
            throw std::runtime_error("Failed to open log file: " + filename);   
        }
    }

    void write(const std::string& message) override{
        file_ << message;
        file_.flush();
    }
};

