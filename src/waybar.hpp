#include <array>
#include <csignal>
#include <memory>
#include "config.hpp"
#include <signal.h>
#include "utils.hpp"
#include <vector>

using namespace std::chrono_literals;

// globals and constants
static bool g_interruptRequest = false;

// TYPES
struct monitor_info_t {
    std::string name{};
    int x_coord{}, y_coord{}, width{}, height{};
    bool hidden = false;

    bool operator<(const monitor_info_t& other) const {
        return x_coord < other.x_coord && y_coord < other.y_coord;
    }

    bool operator==(const monitor_info_t& other) const {
        return name == other.name && x_coord == other.x_coord && y_coord == other.y_coord;
    }
};

enum class BarMode : std::uint8_t {
    HIDE_ALL,
    HIDE_FOCUSED,
    HIDE_MON,
    NONE
};

class Waybar {
    public:
        Waybar(const std::string &mode, int threshold, bool verbose);
        auto run() -> void; // calls the apropiate operation mode
        auto reload() const -> void; // sigusr2
        auto setBarMode(BarMode mode); // setter for mode
    private:
        auto hideAllMonitors(bool is_visible = true) const -> void;
        auto hideFocused() -> void;                  
        auto hideCustom() -> void;
        auto initConfigPath() const -> fs::path;     // retrieves original config filepath
        auto initFallBackConfig() const -> fs::path; // retrieves a fallback config filepath
        auto initPid() const -> pid_t;               // retreives pid of waybar
        auto parseMode(const std::string &mode) -> BarMode;
        auto getVisibleMonitors() const -> Json::Value; // retrieves current monitor
        auto getMonitor(const std::string &name) -> monitor_info_t&; // retrieves the monitor info by a name

        static void handleSignal(int signal) {
            if (signal == SIGINT || signal == SIGTERM || signal == SIGHUP) {
                Utils::log(Utils::WARN, "Interruption detected, saving resources...\n");
                g_interruptRequest = true;
            }
        }

        pid_t m_waybar_pid;
        BarMode m_original_mode = BarMode::NONE;
        int m_bar_threshold = 50;
        bool m_is_console;
        bool m_is_verbose;
        std::string m_hidemon{}; // for mode BarMode::HIDE_MON
        std::vector<monitor_info_t> m_outputs{};
        std::unique_ptr<config> m_config;
};

static auto printHelp() -> void {
    using namespace fmt;

    struct Flag {
        std::string_view name;
        std::string_view description;
    };

    print(fg(color::yellow) | emphasis::bold, "autowaybar: \n");
    print(fg(color::cyan), "Program to manage visibility modes for waybar in Hyprland\n\n");
    print(fg(color::yellow) | emphasis::bold, "Usage:\n");

    print(fg(color::cyan), "  autowaybar ");
    print(fg(color::magenta) | emphasis::bold, "-m");
    print(fg(color::cyan), "/");
    print(fg(color::magenta) | emphasis::bold, "--mode ");
    print(fg(color::white), "<Mode> \n");

    constexpr std::array<Flag, 4> flags = {{
        {.name = "-m --mode", .description = "Select the operation mode for waybar."},
        {.name = "-t --threshold", .description = "Threshold in pixels that should match your waybar width"},
        {.name = "-h --help", .description = "Show this help"},
        {.name = "-v --verbose", .description = "Enable verbose output"}
    }};

    size_t maxFlagLength = 0;
    for (const auto& flag : flags) maxFlagLength = std::max(maxFlagLength, flag.name.length());

    print(fg(color::yellow) | emphasis::bold, "Flags:\n");
    for (const auto& flag : flags) {
        print(fg(color::magenta) | emphasis::bold, "  {:<{}}", flag.name, maxFlagLength + 2);
        print("  {}\n", flag.description);
    }

    // examples
    print("\n");
    print(fg(color::yellow) | emphasis::bold, "Examples:\n");
    print(fg(color::cyan), "  autowaybar -m focused -v\n");
    print(fg(color::cyan), "  autowaybar -m all\n");
    print(fg(color::cyan), "  autowaybar -m mon:DP-2 -v\n");
    print(fg(color::cyan), "  autowaybar -m focused -t 100\n");
    print(fg(color::cyan), "  autowaybar -m all -t 100\n");

    // Detailed mode descriptions
    print(fg(color::yellow) | emphasis::bold, "\nMode:\n");
    print(fg(color::cyan), "  focused: ");
    print(emphasis::italic, "Hide the focused monitor and show the rest. When the mouse reaches the top,\n"
    "  it will show the current monitor, same as `all` mode. (If only 1 monitor is active, it will fallback to `all` mode.)\n\n");
    print(fg(color::cyan), "  all: ");
    print(emphasis::italic, "Hide all monitors, when the mouse reaches the top of the screen, \n"
    "  both will be shown and when you go down the `threshold`, they will be hidden again.\n\n");
    print(fg(color::cyan), "  mon:<monitorname>: ");
    print(emphasis::italic, "Hide the bar only on the specified monitor.\n\n");
}
