#include <array>
#include <csignal>
#include <filesystem>
#include <json/json.h>
#include <json/value.h>
#include <signal.h>
#include "utils.hpp"
#include <vector>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

// globals and constants
static bool g_interruptRequest = false;

const fs::path g_HOMEDIR = std::string(std::getenv("HOME"));

const std::array<fs::path, 3> g_possible_config_lookup = {
    g_HOMEDIR / ".config/waybar/config",
    g_HOMEDIR / "waybar/config",
    "etc/xdg/waybar/config"
};

// TYPES
struct monitor_info_t {
    std::string name{};
    int x_coord{}, width{};
    bool hidden = false;

    bool operator<(monitor_info_t& other) {
        return this->x_coord < other.x_coord;
    }

    bool operator==(monitor_info_t& other) {
        return name == other.name && x_coord == other.x_coord;
    }
};

enum class BarMode : std::uint8_t {
    HIDE_ALL,
    HIDE_FOCUSED,
    NONE
};

class Waybar {
    public:
        Waybar(BarMode mode);
        Waybar(BarMode mode, int threshold);

        auto run() -> void; // calls the apropiate operation mode
        auto reload() -> void; // sigusr2
        auto setBarMode(BarMode mode);
    private:
        auto hideAllMonitors() -> void;
        auto hideFocused() -> void;
        auto initConfigPath() -> fs::path;
        auto initFallBackConfig() -> fs::path;
        auto initPid() -> pid_t;

        static void handleSignal(int signal) {
            if (signal == SIGINT || signal == SIGTERM || signal == SIGHUP) {
                Utils::log(Utils::WARN, "Interruption detected, saving resources...\n");
                g_interruptRequest = true;
            }
        }

        pid_t m_waybar_pid;
        BarMode m_original_mode = BarMode::NONE;
        int m_bar_threshold = 50;
        fs::path m_config_path{};
        std::vector<monitor_info_t> m_outputs{};
};
