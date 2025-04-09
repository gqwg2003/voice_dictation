#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

Logger::Logger()
    : m_logLevel(LogLevel::Info),
      m_loggingEnabled(false)
{
}

Logger::~Logger()
{
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

bool Logger::init(const std::string& logFilePath)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close previous log file if open
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    
    // Create log directory if it doesn't exist
    QDir logDir = QFileInfo(QString::fromStdString(logFilePath)).dir();
    if (!logDir.exists()) {
        if (!logDir.mkpath(".")) {
            std::cerr << "Failed to create log directory: " << logDir.path().toStdString() << std::endl;
            return false;
        }
    }
    
    // Open log file
    m_logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
        return false;
    }
    
    // Write header to log file
    m_logFile << "\n\n--- Log started at " 
              << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString() 
              << " ---\n";
    m_logFile.flush();
    
    return true;
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Error, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::Warning, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::Info, message);
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::Debug, message);
}

void Logger::verbose(const std::string& message)
{
    log(LogLevel::Verbose, message);
}

void Logger::setLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logLevel = level;
}

LogLevel Logger::getLogLevel() const
{
    return m_logLevel;
}

void Logger::setEnableLogging(bool enable)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loggingEnabled = enable;
}

bool Logger::isLoggingEnabled() const
{
    return m_loggingEnabled;
}

void Logger::log(LogLevel level, const std::string& message)
{
    // Check if logging is enabled and if level is sufficient
    if (!m_loggingEnabled || level > m_logLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get current timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    
    // Format log message
    std::string logMessage = timestamp.toStdString() + " [" + levelToString(level) + "] " + message;
    
    // Write to log file if open
    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
        m_logFile.flush();
    }
    
    // Always print to standard output for debugging
    std::cout << logMessage << std::endl;
}

std::string Logger::levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Warning:
            return "WARNING";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Verbose:
            return "VERBOSE";
        default:
            return "UNKNOWN";
    }
} 