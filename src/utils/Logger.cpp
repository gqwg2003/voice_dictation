#include "Logger.h"
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>

// Constructor
Logger::Logger(LogLevel level) : m_level(level) {
    namespace fs = std::filesystem;
    
    // Create logs directory if it doesn't exist
    fs::path logsDir = "logs";
    if (!fs::exists(logsDir)) {
        fs::create_directory(logsDir);
    }
    
    // Generate log file name based on current time
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "logs/voice_dictation_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".log";
    m_logFilePath = ss.str();
    
    // Open log file
    m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);
    
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << m_logFilePath << std::endl;
    }
    
    // Log startup message
    info("Logger initialized");
}

// Destructor
Logger::~Logger() {
    if (m_logFile.is_open()) {
        info("Logger shutting down");
        m_logFile.close();
    }
}

// Log level-specific methods
void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

// Set log level
void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
    info("Log level set to " + getLevelString(level));
}

// Get current log level
LogLevel Logger::getLevel() const {
    return m_level;
}

// Internal log method
void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_level) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string logMessage = getTimestamp() + " [" + getLevelString(level) + "] " + message;
    
    // Write to console
    std::cout << logMessage << std::endl;
    
    // Write to log file
    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
        m_logFile.flush();
    }
}

// Get current timestamp
std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Get string representation of log level
std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
} 