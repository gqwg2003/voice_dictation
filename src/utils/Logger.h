#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    Logger(LogLevel level = LogLevel::INFO);
    ~Logger();

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void setLevel(LogLevel level);
    LogLevel getLevel() const;

private:
    void log(LogLevel level, const std::string& message);
    std::string getTimestamp();
    std::string getLevelString(LogLevel level);

private:
    LogLevel m_level;
    std::ofstream m_logFile;
    std::mutex m_mutex;
    std::string m_logFilePath;
}; 