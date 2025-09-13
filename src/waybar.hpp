#include <array>
#include <csignal>
#include <memory>
#include <atomic>
#include "json/value.h"
#include "json/reader.h"
#include "json/writer.h"
#include <filesystem>
#include <fstream>
#include <signal.h>
#include "utils.hpp"
#include <vector>
#include <iomanip>

using namespace std::chrono_literals;

// Forward declarations
class Waybar;
inline auto printHelp() -> void;

// Configuration constants
namespace Constants {
    constexpr int DEFAULT_BAR_THRESHOLD = 100;
    constexpr int MOUSE_ACTIVATION_ZONE = 1;  // pixels from top of monitor
    constexpr auto POLLING_INTERVAL = 80ms;   // mouse position polling frequency
    constexpr int MIN_THRESHOLD = 1;          // minimum threshold value
    constexpr int MAX_THRESHOLD = 1000;       // maximum threshold value
    constexpr int MONITOR_MODE_PREFIX_LENGTH = 4;  // "mon:" prefix length
    constexpr int SINGLE_MONITOR_THRESHOLD = 1;    // fallback threshold for single monitor
    constexpr int CONFIG_FLAG_COUNT = 4;           // number of command line flags
    constexpr auto WORKSPACE_SHOW_DURATION = 1000ms;   // how long to show waybar after workspace change
    constexpr auto MOUSE_ACTIVATION_DELAY = 250ms; // how long mouse must be in activation zone
    constexpr int MAX_WAYBAR_CRASHES = 3;          // maximum waybar crashes before giving up
    constexpr auto WAYBAR_CRASH_WINDOW = 30s;      // time window for crash counting
    constexpr auto ENVIRONMENT_RETRY_INTERVAL = 10s; // how long to wait between environment checks
    constexpr auto ENVIRONMENT_RETRY_TIMEOUT = 10min; // how long to keep trying before giving up
}

// TYPES
struct monitor_info_t {
    std::string name{};
    int x_coord{}, y_coord{}, width{}, height{};
    bool hidden = false;

    // for sorting - improved comparison logic
    bool operator<(const monitor_info_t& other) const {
        // Primary sort by x-coordinate, secondary by y-coordinate
        if (x_coord != other.x_coord) {
            return x_coord < other.x_coord;
        }
        return y_coord < other.y_coord;
    }
    
    // Add equality operator for completeness
    bool operator==(const monitor_info_t& other) const {
        return name == other.name && 
               x_coord == other.x_coord && 
               y_coord == other.y_coord &&
               width == other.width && 
               height == other.height;
    }
};

enum class BarMode : std::uint8_t {
    HIDE_ALL,
    HIDE_FOCUSED,
    HIDE_MON
};

class Waybar {
public:
    Waybar(const std::string &mode, int threshold, int verbose, const std::string &config_dir);
    ~Waybar();
    auto run() -> void; // calls the apropiate operation mode
    auto reloadPid() -> void; // sigusr2
    auto restoreOriginal() -> void; // restore original waybar config
    auto setBarMode(BarMode mode); // setter for mode
    auto shutdown() -> void; // properly terminate waybar process
private:
    // modes
    auto hideAllMonitors(bool is_visible = true) -> void;
    auto hideFocused() -> void;                  
    auto hideCustom() -> void;
    auto parseMode(const std::string &mode) -> BarMode;
    auto runFocusedMode() -> void;
    auto runCustomMode() -> void;
    auto validateMonitorExists() -> void;
    
    // custom mode helpers
    auto setupCustomMode() -> void;
    auto runCustomModeLoop() -> void;
    auto initializeCustomModeMouse() -> std::pair<int, int>;
    auto processCustomModeIteration(int mouse_x, int mouse_y) -> bool;
    auto showHiddenMonitor(monitor_info_t& mon) -> bool;
    auto cleanupCustomMode() -> void;
    auto handleMonitorThreshold(monitor_info_t& mon, int& mouse_x, int& mouse_y, int local_bar_threshold) -> bool;
    
    // focused mode helpers
    auto setupFocusedMode() -> void;
    auto validateFocusedModeConfig() -> void;
    auto runFocusedModeLoop() -> void;
    auto initializeMousePosition() -> std::pair<int, int>;
    auto applyChanges(bool need_reload) -> void;
    auto sleepAndUpdateMouse(int& mouse_x, int& mouse_y) -> void;
    auto cleanupFocusedMode() -> void;
    auto processFocusedMonitors(int mouse_x, int mouse_y) -> bool;
    auto isCursorInCurrentMonitor(const monitor_info_t& mon, int mouse_x, int mouse_y) -> bool;
    auto processCurrentMonitor(monitor_info_t& mon, int mouse_x, int mouse_y) -> bool;
    auto handleVisibleMonitor(monitor_info_t& mon, int mouse_x, int mouse_y) -> bool;
    auto handleHiddenMonitor(monitor_info_t& mon, int mouse_x, int mouse_y) -> bool;
    
    // all monitors mode helpers
    auto setupAllMonitorsMode(bool& is_visible) -> void;
    auto runAllMonitorsLoop(bool is_visible) -> void;
    auto cleanupAllMonitorsMode() -> void;
    auto processAllMonitorsVisibility(int root_x, int root_y, bool is_visible) -> bool;
    auto processMonitorVisibility(const monitor_info_t& mon, int root_y, bool is_visible) -> bool;
    auto showWaybarAndKeepOpen(const monitor_info_t& mon, int local_bar_threshold) -> bool;
    auto hideWaybarAndReturnFalse() -> bool;
    auto shouldShowWaybar(const monitor_info_t& mon, int root_y) const -> bool;
    auto shouldHideWaybar(const monitor_info_t& mon, int root_y, int threshold) const -> bool;
    auto checkMouseActivationDelay() -> bool;
    auto showWaybar() -> void;
    auto hideWaybar() -> void;
    
    // workspace monitoring helpers
    auto getCurrentWorkspace() const -> int;
    auto checkWorkspaceChange() const -> bool;
    auto handleWorkspaceChange() -> void;

    // monitors
    auto getMonitor(const std::string &name) -> monitor_info_t&; // retrieves the monitor info by a name
    auto requestApplyVisibleMonitors(bool need_reload) -> void; 

    // misc
    auto initPid() const -> pid_t;               // retreives pid of waybar
    auto initPidOrRestart() -> pid_t;           // gets pid or restarts waybar if not running
    auto restartWaybar() -> pid_t;               // restarts waybar process
    auto checkWaybarCrashLimit() -> bool;       // checks if waybar has crashed too many times
    auto enforceSingleWaybar() -> void;         // enforces single waybar policy
    auto isEnvironmentReady() -> bool;          // checks if Hyprland/Wayland environment is ready
    auto waitForEnvironmentReady() -> bool;     // waits for environment to be ready with retry logic
    auto initLogFile() -> void;                 // initialize single log file for diagnostics
    auto logToFile(const std::string& message) -> void; // write message to log file
    
    // initialization
    auto initialize() -> void;
    
    // config management
    auto initConfig() -> void;
    auto findConfigPath() -> std::string;
    auto loadConfig() -> void;
    auto validateConfig() -> void;
    auto getConfigPath() -> std::string;
    auto isValidConfigPath(const std::string& path) const -> bool;
    auto saveConfig() -> void;
    auto getOutputs() -> Json::Value&;
    auto setOutputs(const Json::Value &outputs) -> void;
    auto handleSignal(int signal) -> void {
        if (signal == SIGINT || signal == SIGTERM || signal == SIGHUP) {
            log_message(WARN, "Interruption detected, saving resources...\n");
            // Note: This function is no longer used since we use global signal handlers
        }
    }
    static void cleanupSignals() {
        std::signal(SIGINT, SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);
        std::signal(SIGHUP, SIG_DFL);
    }

    pid_t m_waybar_pid;
    BarMode m_original_mode = BarMode::HIDE_ALL;
    bool m_is_console;
    int m_verbose_level;
    int m_bar_threshold = Constants::DEFAULT_BAR_THRESHOLD;
    bool m_waybar_visible = false;  // track current waybar visibility state
    std::chrono::steady_clock::time_point m_mouse_activation_start{}; // when mouse entered activation zone
    bool m_mouse_in_activation_zone = false; // track if mouse is currently in activation zone
    std::string m_hidemon{}; // for mode BarMode::HIDE_MON
    std::vector<monitor_info_t> m_outputs{};
    std::string m_config_path;
    std::string m_config_dir;
    Json::Value m_config;
    Json::Value m_backup;
    
    // Waybar crash tracking
    int m_waybar_crash_count = 0;
    std::chrono::steady_clock::time_point m_crash_window_start{};
    
    // Environment readiness tracking
    int m_environment_retry_count = 0;
    std::chrono::steady_clock::time_point m_environment_retry_start{};
    
    // Logging
    std::string m_log_file_path;
    std::ofstream m_log_file;
};

inline auto printHelp() -> void {
    using namespace fmt;

    struct Flag {
        std::string_view name;
        std::string_view description;
    };

    print(fg(color::yellow) | emphasis::bold, "autowaybar-ai v1.1.2: \n");
    print(fg(color::cyan), "AI-enhanced program to manage visibility modes for waybar in Hyprland\n\n");
    print(fg(color::yellow) | emphasis::bold, "Usage:\n");

    print(fg(color::cyan), "  autowaybar ");
    print(fg(color::magenta) | emphasis::bold, "[-m");
    print(fg(color::cyan), "/");
    print(fg(color::magenta) | emphasis::bold, "--mode ");
    print(fg(color::white), "<Mode>] \n");

    constexpr std::array<Flag, Constants::CONFIG_FLAG_COUNT> flags = {{
        {.name = "-m --mode", .description = "Select the operation mode for waybar (default: all)."},
        {.name = "-t --threshold", .description = "Threshold in pixels that should match your waybar width"},
        {.name = "-h --help", .description = "Show this help"},
        {.name = "-v --verbose", .description = "Enable verbose output (-v for LOG level, -vv for TRACE level)"}
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
    print(fg(color::cyan), "  autowaybar -m mon:DP-2,HDMI-1 -v\n");
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
    print(emphasis::italic, "Hide the bar only on the specified monitor(s).\n");
    print(emphasis::italic, "  Multiple monitors can be specified: mon:DP-2,HDMI-1\n\n");
}
