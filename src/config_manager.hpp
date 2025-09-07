#pragma once

#include "raii_wrappers.hpp"
#include "json/value.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// RAII manager for configuration operations
class ConfigManager {
private:
    std::string m_path;
    Json::Value m_config;
    Json::Value m_backup;
    bool m_modified = false;
    bool m_initialized = false;

public:
    explicit ConfigManager(const std::string& path) : m_path(path) {
        loadConfig();
        m_backup = m_config;
        m_initialized = true;
    }
    
    ~ConfigManager() {
        if (m_initialized && m_modified) {
            try {
                restoreOriginal();
            } catch (const std::exception& e) {
                Utils::log(Utils::ERR, "Failed to restore original config during destruction: {}", e.what());
            }
        }
    }
    
    void loadConfig() {
        FileWrapper file(m_path);
        try {
            file.get() >> m_config;
        } catch (const std::exception& e) {
            Utils::log(Utils::CRIT, "Invalid waybar json file at {}", m_path);
            throw std::runtime_error("Invalid waybar json file at " + m_path);
        }
    }
    
    void saveConfig() {
        OutputFileWrapper file(m_path, std::iostream::trunc);
        file.get() << m_config;
        m_modified = true;
    }
    
    void restoreOriginal() {
        OutputFileWrapper file(m_path, std::iostream::trunc);
        file.get() << m_backup;
        m_modified = false;
    }
    
    void setOutputs(const Json::Value& outputs) {
        m_config["output"] = outputs;
        saveConfig();
    }
    
    Json::Value& getConfig() { return m_config; }
    const Json::Value& getBackup() const { return m_backup; }
    Json::Value& getOutputs() { return m_config["output"]; }
    const std::string& getPath() const { return m_path; }
    bool isModified() const { return m_modified; }
    
    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Allow moving
    ConfigManager(ConfigManager&& other) noexcept 
        : m_path(std::move(other.m_path)), 
          m_config(std::move(other.m_config)), 
          m_backup(std::move(other.m_backup)),
          m_modified(other.m_modified),
          m_initialized(other.m_initialized) {
        other.m_initialized = false;
        other.m_modified = false;
    }
    
    ConfigManager& operator=(ConfigManager&& other) noexcept {
        if (this != &other) {
            if (m_initialized && m_modified) {
                try {
                    restoreOriginal();
                } catch (const std::exception& e) {
                    Utils::log(Utils::ERR, "Failed to restore original config during move: {}", e.what());
                }
            }
            m_path = std::move(other.m_path);
            m_config = std::move(other.m_config);
            m_backup = std::move(other.m_backup);
            m_modified = other.m_modified;
            m_initialized = other.m_initialized;
            other.m_initialized = false;
            other.m_modified = false;
        }
        return *this;
    }
};
