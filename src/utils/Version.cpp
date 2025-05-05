#include "Version.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Constructor
Version::Version() {
    try {
        m_currentVersion = loadVersionFromCache();
    } catch (const std::exception& e) {
        std::cerr << "Failed to load version from cache: " << e.what() << std::endl;
        m_currentVersion = getVersionFromGit();
        saveVersionToCache(m_currentVersion);
    }
}

// Destructor
Version::~Version() {
    // Nothing to do here
}

// Get current version
VersionData Version::getVersion() const {
    return m_currentVersion;
}

// Force update version from Git
VersionData Version::forceUpdateVersion() {
    try {
        m_currentVersion = getVersionFromGit();
        saveVersionToCache(m_currentVersion);
    } catch (const std::exception& e) {
        std::cerr << "Failed to update version from Git: " << e.what() << std::endl;
        // Try to load from cache as fallback
        try {
            m_currentVersion = loadVersionFromCache();
        } catch (const std::exception& e) {
            // Set default values if all else fails
            m_currentVersion.displayVersion = "3.5.0";
            m_currentVersion.buildNumber = "1";
            m_currentVersion.commitHash = "unknown";
            m_currentVersion.buildDate = "unknown";
            m_currentVersion.buildTime = "unknown";
        }
    }
    return m_currentVersion;
}

// Get runtime version string
std::string Version::getRuntimeVersion() {
    VersionData version;
    try {
        Version v;
        version = v.getVersion();
    } catch (const std::exception& e) {
        return "3.5.0";
    }
    return version.displayVersion;
}

// Get runtime application info
std::map<std::string, std::string> Version::getRuntimeAppInfo() {
    std::map<std::string, std::string> info;
    try {
        Version v;
        VersionData version = v.getVersion();
        
        info["version"] = version.displayVersion;
        info["build"] = version.buildNumber;
        info["commit"] = version.commitHash;
        info["build_date"] = version.buildDate;
        info["build_time"] = version.buildTime;
        
        // Add feature flags
        for (const auto& feature : version.features) {
            info["feature_" + feature.first] = feature.second ? "enabled" : "disabled";
        }
        
    } catch (const std::exception& e) {
        info["version"] = "3.5.0";
        info["build"] = "1";
        info["error"] = e.what();
    }
    return info;
}

// Load version from cache file
VersionData Version::loadVersionFromCache() const {
    VersionData version;
    
    if (!std::filesystem::exists(VERSION_CACHE_FILE)) {
        throw std::runtime_error("Version cache file does not exist");
    }
    
    std::ifstream file(VERSION_CACHE_FILE);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open version cache file");
    }
    
    json j;
    file >> j;
    
    version.displayVersion = j.value("display_version", "3.5.0");
    version.buildNumber = j.value("build_number", "1");
    version.commitHash = j.value("commit_hash", "unknown");
    version.buildDate = j.value("build_date", "unknown");
    version.buildTime = j.value("build_time", "unknown");
    
    // Load features
    if (j.contains("features") && j["features"].is_object()) {
        for (auto& [key, value] : j["features"].items()) {
            version.features[key] = value.get<bool>();
        }
    }
    
    return version;
}

// Save version to cache file
void Version::saveVersionToCache(const VersionData& version) const {
    json j;
    
    j["display_version"] = version.displayVersion;
    j["build_number"] = version.buildNumber;
    j["commit_hash"] = version.commitHash;
    j["build_date"] = version.buildDate;
    j["build_time"] = version.buildTime;
    
    // Save features
    json features;
    for (const auto& [key, value] : version.features) {
        features[key] = value;
    }
    j["features"] = features;
    
    std::ofstream file(VERSION_CACHE_FILE);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open version cache file for writing");
    }
    
    file << j.dump(4);
}

// Get version from Git
VersionData Version::getVersionFromGit() const {
    VersionData version;
    
    try {
        // Try to get the latest git tag
        std::string gitTag = executeCommand("git describe --tags --abbrev=0 2>/dev/null");
        if (gitTag.empty()) {
            gitTag = "v3.5.0";
        }
        
        // Remove 'v' prefix if present
        if (gitTag[0] == 'v') {
            gitTag = gitTag.substr(1);
        }
        
        // Get commit count since tag
        std::string commitCount = executeCommand("git rev-list --count HEAD 2>/dev/null");
        if (commitCount.empty()) {
            commitCount = "1";
        }
        
        // Get commit hash
        std::string commitHash = executeCommand("git rev-parse --short HEAD 2>/dev/null");
        if (commitHash.empty()) {
            commitHash = "unknown";
        }
        
        // Set version data
        version.displayVersion = gitTag;
        version.buildNumber = commitCount;
        version.commitHash = commitHash;
        
        // Get current date and time for build timestamp
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream dateStream;
        dateStream << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
        version.buildDate = dateStream.str();
        
        std::stringstream timeStream;
        timeStream << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
        version.buildTime = timeStream.str();
        
        // Set default features
        version.features["multilingual"] = true;
        version.features["audio_visualization"] = true;
        version.features["clipboard_integration"] = true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting version from Git: " << e.what() << std::endl;
        throw;
    }
    
    return version;
}

// Execute a command and get output
std::string Version::executeCommand(const std::string& command) const {
    std::array<char, 128> buffer;
    std::string result;
    
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif
    
    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    // Trim trailing newlines
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    
    return result;
} 