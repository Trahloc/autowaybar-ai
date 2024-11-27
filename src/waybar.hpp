#include <csignal>
#include "utils.hpp"
#include <json/json.h>
#include <filesystem>
#include <json/value.h>
#include <signal.h>
#include <vector>
#include <array>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

// GLOBALS and CONSTANS
static bool interruptRequest = false;
const fs::path HOMEDIR = std::string(std::getenv("HOME"));
const std::array<fs::path, 2> possible_config_lookup = {
    HOMEDIR / ".config/ml4w/settings/waybar-theme.sh",
    HOMEDIR / ".cache/.themestyle.sh"
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
        auto getCurrentML4WConfig() -> fs::path;

        static void handleSignal(int signal) {
            if (signal == SIGINT) {
                Utils::log(Utils::WARN, "Interruption detected, saving resources...\n");
                interruptRequest = true;
            }
        }

        pid_t waybar_pid;
        const int bar_threshold = 43;
        fs::path full_config;
        std::vector<monitor_info> outputs;
};