#pragma once

#include <string>
#include <map>

// Structure to hold version information
struct VersionData {
    std::string displayVersion;
    std::string buildNumber;
    std::string commitHash;
    std::string buildDate;
    std::string buildTime;
    std::map<std::string, bool> features;
};

class Version {
public:
    Version();
    ~Version();

    // Get current version
    VersionData getVersion() const;
    
    // Force update version from Git
    VersionData forceUpdateVersion();
    
    // Get runtime information
    static std::string getRuntimeVersion();
    static std::map<std::string, std::string> getRuntimeAppInfo();

private:
    // Load version from cache file
    VersionData loadVersionFromCache() const;
    
    // Save version to cache file
    void saveVersionToCache(const VersionData& version) const;
    
    // Get version from Git
    VersionData getVersionFromGit() const;
    
    // Execute a command and get output
    std::string executeCommand(const std::string& command) const;
    
private:
    const std::string VERSION_CACHE_FILE = "version_cache.json";
    VersionData m_currentVersion;
}; 