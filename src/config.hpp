#include "json/json.h"
#include <fstream>
#include "json/value.h"
#include <filesystem>
#include <istream>
#include <sys/types.h>
#include "utils.hpp"

namespace fs = std::filesystem;

const fs::path g_HOMEDIR = std::string(std::getenv("HOME"));

const std::array<fs::path, 3> g_possible_config_lookup = {
    g_HOMEDIR / ".config/waybar/config",
    g_HOMEDIR / "waybar/config",
    "etc/xdg/waybar/config"
};

class config {
public:
    config(pid_t waybarpid);
    auto getConfigPath() -> fs::path;
    auto getOutputs() -> Json::Value&;
    auto setOutputs(const Json::Value &outputs) -> void;
    auto restoreOriginal() -> void;
    auto init() -> void;
private:
    auto saveConfig() -> void;
    auto initFallBackConfig() const -> fs::path;

    fs::path m_config_path;    
    Json::Value m_config;
    Json::Value m_backup;
    pid_t m_waybar_pid;
};
