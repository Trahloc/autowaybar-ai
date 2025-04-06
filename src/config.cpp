#include "config.hpp"
#include <exception>
#include <fstream>
#include <sched.h>
#include "utils.hpp"

config::config(pid_t waybarpid) {
    m_waybar_pid = waybarpid;
}

auto config::init() -> void {
    m_config_path = getConfigPath();
    if (m_config_path.empty()) 
        throw std::runtime_error("Unable to find Waybar config.\n");

    Utils::log(Utils::INFO, "Waybar config file found in '{}'\n", m_config_path.string());

    std::ifstream file(m_config_path);
    if (!file) throw std::runtime_error("[CRIT] Couldn't open config file.\n");

    try {
        file >> m_config;
    } catch (std::exception e) {
        Utils::log(Utils::CRIT, "Invalid waybar json file at {}", m_config_path.string()); 
        file.close();
        std::exit(EXIT_FAILURE);
    }
    file.close();

    Utils::log(Utils::LOG, "Backuping original config.\n");
    m_backup = m_config; 
  
    if (m_config.isArray()) {
        Utils::log(Utils::CRIT, "Multiple bars are not supported.\n");
        std::exit(EXIT_FAILURE);
    }
}

auto config::saveConfig() -> void {
    std::ofstream file(m_config_path, std::iostream::trunc);
    if (!file.is_open())
        throw std::runtime_error("[ERR] Couldn't open the file.\n");
    file << m_config;
    file.close();
}

auto config::setOutputs(const Json::Value &outputs) -> void {
    m_config["output"] = outputs;
    saveConfig();
}

auto config::restoreOriginal() -> void {
    std::ofstream file(m_config_path, std::iostream::trunc);
    if (!file.is_open())
        throw std::runtime_error("[ERR] Couldn't open the file.\n");
    file << m_backup;
    file.close();
}


auto config::getOutputs() -> Json::Value& {
    return m_config["output"];
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
