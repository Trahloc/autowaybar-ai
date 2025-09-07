#include "json/value.h"
#include <filesystem>
#include <sys/types.h>
#include <memory>

namespace fs = std::filesystem;

const fs::path g_HOMEDIR = []() {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return fs::path(home);
}();

const std::array<fs::path, 3> g_possible_config_lookup = {
    g_HOMEDIR / ".config/waybar/config",
    g_HOMEDIR / "waybar/config",
    "etc/xdg/waybar/config"
};

// Forward declaration
class ConfigManager;

class config {
public:
    config(pid_t waybarpid);
    ~config();
    auto getConfigPath() -> fs::path;
    auto getOutputs() -> Json::Value&;
    auto setOutputs(const Json::Value &outputs) -> void;
    auto restoreOriginal() -> void;
    auto init() -> void;
private:
    auto initFallBackConfig() const -> fs::path;

    fs::path m_config_path;    
    ConfigManager* m_config_manager;
    pid_t m_waybar_pid;
};
