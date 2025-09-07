#include "config.hpp"
#include "config_manager.hpp"
#include <exception>
#include <fstream>
#include <sched.h>
#include "utils.hpp"

config::config(pid_t waybarpid) : m_config_manager(nullptr) {
    m_waybar_pid = waybarpid;
}

config::~config() {
    if (m_config_manager) {
        delete m_config_manager;
        m_config_manager = nullptr;
    }
}

auto config::init() -> void {
    m_config_path = getConfigPath();
    if (m_config_path.empty()) 
        throw std::runtime_error("Unable to find Waybar config.\n");

    Utils::log(Utils::INFO, "Waybar config file found in '{}'\n", m_config_path.string());

    try {
        m_config_manager = new ConfigManager(m_config_path.string());
    } catch (const std::exception& e) {
        Utils::log(Utils::CRIT, "Failed to initialize config manager: {}", e.what());
        std::exit(EXIT_FAILURE);
    }

    Utils::log(Utils::LOG, "Backuping original config.\n");
  
    if (m_config_manager->getConfig().isArray()) {
        Utils::log(Utils::CRIT, "Multiple bars are not supported.\n");
        std::exit(EXIT_FAILURE);
    }
}

// Modifies the config file with the given outputs
// you still need to reload waybar pid to see changes
auto config::setOutputs(const Json::Value &outputs) -> void {
    if (!m_config_manager) {
        throw std::runtime_error("Config manager not initialized");
    }
    m_config_manager->setOutputs(outputs);
}

// restores the original config to disk
auto config::restoreOriginal() -> void {
    if (!m_config_manager) {
        throw std::runtime_error("Config manager not initialized");
    }
    m_config_manager->restoreOriginal();
}

auto config::getOutputs() -> Json::Value& {
    if (!m_config_manager) {
        throw std::runtime_error("Config manager not initialized");
    }
    return m_config_manager->getOutputs();
}

auto config::initFallBackConfig() const  -> fs::path {
    for (auto path : g_possible_config_lookup) {
        if (fs::exists(path) || 
            fs::exists(path.replace_extension(fs::path{".jsonc"})))
            return path;
    }
    return {};
}

auto config::getConfigPath() -> fs::path {
    auto str_cmd = Utils::getProcArgs(m_waybar_pid);

    auto find = str_cmd.find("-c");
    if (find != std::string::npos) {
        str_cmd.erase(str_cmd.begin(), str_cmd.begin() + find + 2);
        find = str_cmd.find("-s");

        if (find != std::string::npos) {
            str_cmd.erase(str_cmd.begin() + find, str_cmd.end());

            // strip special characters
            str_cmd.erase(
                std::remove(str_cmd.begin(), str_cmd.end(), '\000'),
                str_cmd.end()
            );

            return str_cmd;
        }
    }

    // search default config paths
    return initFallBackConfig();
}
