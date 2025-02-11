#include <csignal>
#include <json/json.h>
#include <filesystem>
#include <json/value.h>
#include <signal.h>
#include <vector>
#include <array>
#include "utils.hpp"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

// GLOBALS and CONSTANS
static bool g_interruptRequest = false;
const fs::path g_HOMEDIR = std::string(std::getenv("HOME"));
const std::array<fs::path, 2> g_possible_config_lookup = {
    g_HOMEDIR / ".config/waybar/config.jsonc", // TODO: PUT THE DEFAULT PATHS
    g_HOMEDIR / ".cache/.themestyle.sh"
};

// TYPES
struct monitor_info_t {
    std::string name;
    int x_coord, width;
    bool hidden = false;

    bool operator<(monitor_info_t& other) {
        return this->x_coord < other.x_coord;
    }

    bool operator==(monitor_info_t& other) {
        return name == other.name && x_coord == other.x_coord;
    }
};

enum class BarMode {
    HIDE_ALL,
    HIDE_UNFOCUSED
};

class Waybar {
    public:
        Waybar();

        auto run(BarMode mode) -> void; // calls the apropiate operation mode
        auto reload() -> void; // sigusr2

    private:
        auto hideAllMonitors() -> void;
        auto hideUnfocused() -> void;
        auto getConfigPath() -> fs::path; // TODO: IMPLEMENT

        static void handleSignal(int signal) {
            if (signal == SIGINT || signal == SIGTERM) {
                Utils::log(Utils::WARN, "Interruption detected, saving resources...\n");
                g_interruptRequest = true;
            }
        }

        pid_t m_waybar_pid;
        const int m_bar_threshold = 43;
        fs::path m_config_path;
        std::vector<monitor_info_t> m_outputs;
};
