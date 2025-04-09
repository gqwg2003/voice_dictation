#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iostream>
#include <QString>

enum class LogLevel {
    Error = 0,
    Warning = 1,
    Info = 2,
    Debug = 3,
    Verbose = 4
};

class Logger {
public:
    Logger();
    ~Logger();
    
    // Initialize logger with path to log file
    bool init(const std::string& logFilePath);
    
    // Log methods for different severity levels
    void error(const std::string& message);
    void warning(const std::string& message);
    void info(const std::string& message);
    void debug(const std::string& message);
    void verbose(const std::string& message);
    
    // Configuration
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;
    void setEnableLogging(bool enable);
    bool isLoggingEnabled() const;

private:
    // Internal logging method
    void log(LogLevel level, const std::string& message);
    
    // Get string representation of log level
    std::string levelToString(LogLevel level);
    
    // Log file handle
    std::ofstream m_logFile;
    
    // Mutex for thread-safe logging
    std::mutex m_mutex;
    
    // Configuration
    LogLevel m_logLevel;
    bool m_loggingEnabled;
}; 