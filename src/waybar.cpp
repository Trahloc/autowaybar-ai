#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <stdexcept>
#include <sys/types.h>
#include <thread>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include "utils.hpp"
#include "Hyprland.hpp"
#include <filesystem>

namespace fs = std::filesystem;

// Global interrupt flag for signal handlers
static std::atomic<bool> g_interrupt_request{false};

// Global workspace tracking
static std::atomic<int> g_current_workspace{1};

// Auxiliary functions

auto register_signals(const Waybar* /* instance */) -> void {
    // Set up signal handlers that set the global interrupt flag
    std::signal(SIGINT, [](int) { 
        log_message(WARN, "Interruption detected, saving resources...\n");
        g_interrupt_request.store(true, std::memory_order_release);
    });
    std::signal(SIGTERM, [](int) { 
        log_message(WARN, "Interruption detected, saving resources...\n");
        g_interrupt_request.store(true, std::memory_order_release);
    });
    std::signal(SIGHUP, [](int) { 
        log_message(WARN, "Interruption detected, saving resources...\n");
        g_interrupt_request.store(true, std::memory_order_release);
    });
}

auto is_cursor_in_monitor(const monitor_info_t &mon, int x, int y) -> bool {
    return mon.x_coord <= x &&
           mon.x_coord + mon.width >= x &&
           mon.y_coord <= y &&
           mon.y_coord + mon.height >= y;
}

// Waybar functions

// reloads waybar with the new visible monitors
auto Waybar::requestApplyVisibleMonitors(bool need_reload) -> void {
    if (need_reload) {
        if (m_verbose_level >= 1) {
            log_message(LOG, "Updating\n");
        }
        Json::Value arr(Json::arrayValue);
        for (const auto& mon : m_outputs) {
            if (!mon.hidden)
                arr.append(mon.name);
        }
        setOutputs(arr);
        if (m_verbose_level >= 1) {
            log_message(LOG, "New update: {}", fmt::streamed(getOutputs()));
        }
        reloadPid(); // always reload to apply changes
    }
}

 
auto Waybar::checkWaybarCrashLimit() -> bool {
    auto now = std::chrono::steady_clock::now();
    
    // Reset crash count if window has expired
    if (now - m_crash_window_start > Constants::WAYBAR_CRASH_WINDOW) {
        m_waybar_crash_count = 0;
        m_crash_window_start = now;
    }
    
    // Check if we've exceeded the crash limit
    if (m_waybar_crash_count >= Constants::MAX_WAYBAR_CRASHES) {
        return true; // Too many crashes
    }
    
    return false; // Within limits
}

auto Waybar::restartWaybar() -> pid_t {
    log_message(INFO, "Attempting to restart waybar...\n");
    
    // Check crash limit before attempting restart
    if (checkWaybarCrashLimit()) {
        throw std::runtime_error("Waybar is unstable - crashed 3 times in 30 seconds. Giving up.");
    }
    
    // Increment crash count and set window start if this is the first crash
    if (m_waybar_crash_count == 0) {
        m_crash_window_start = std::chrono::steady_clock::now();
    }
    m_waybar_crash_count++;
    
    // Try to start waybar
    pid_t child_pid = fork();
    if (child_pid == 0) {
        // Child process - exec waybar
        execlp("waybar", "waybar", nullptr);
        // If execlp fails, exit with error
        std::exit(1);
    } else if (child_pid > 0) {
        // Parent process - wait a moment for waybar to start
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Check if waybar is now running
        std::string pid_str = execute_command("pidof waybar");
        if (!pid_str.empty()) {
            pid_str.erase(pid_str.find_last_not_of(" \t\n\r") + 1);
            pid_t actual_pid = std::stoi(pid_str);
            log_message(INFO, "Waybar restarted successfully with PID: {}\n", actual_pid);
            return actual_pid;
        } else {
            throw std::runtime_error("Failed to restart waybar - process not found after startup");
        }
    } else {
        // Fork failed
        throw std::runtime_error("Failed to fork process for waybar restart: " + std::string(strerror(errno)));
    }
}

auto Waybar::initPid() const -> pid_t {
    std::string pid_str = execute_command("pidof waybar");
    if (pid_str.empty()) {
        throw std::runtime_error("Waybar is not running");
    }
    
    pid_str.erase(pid_str.find_last_not_of(" \t\n\r") + 1);
    pid_t pid = std::stoi(pid_str);
    
    return pid;
}

auto Waybar::initPidOrRestart() -> pid_t {
    std::string pid_str = execute_command("pidof waybar");
    if (pid_str.empty()) {
        log_message(INFO, "Waybar not running, attempting to restart...\n");
        return restartWaybar();
    }
    
    pid_str.erase(pid_str.find_last_not_of(" \t\n\r") + 1);
    pid_t pid = std::stoi(pid_str);
    
    return pid;
}

// Parse mode argument
auto Waybar::parseMode(const std::string &mode) -> BarMode {
    // Default to "all" mode if no mode specified
    if (mode.empty()) {
        return BarMode::HIDE_ALL;
    }

    if (mode == "all") return BarMode::HIDE_ALL;
    if (mode == "focused") return BarMode::HIDE_FOCUSED;
    if (mode.length() > Constants::MONITOR_MODE_PREFIX_LENGTH && mode.substr(0, Constants::MONITOR_MODE_PREFIX_LENGTH) == "mon:") {
        m_hidemon = mode.substr(Constants::MONITOR_MODE_PREFIX_LENGTH); 
        return BarMode::HIDE_MON;
    }
    
    log_message(CRIT, "Invalid mode value: {}\n", mode);
    printHelp();
    throw std::invalid_argument("Invalid mode value: " + mode);
}


Waybar::Waybar(const std::string &mode, int threshold, int verbose, const std::string &config_dir)
    : m_waybar_pid(initPidOrRestart()),
      m_original_mode(parseMode(mode)),
      m_is_console(isatty(fileno(stdin))),
      m_verbose_level(verbose),
      m_bar_threshold(threshold),
      m_config_dir(config_dir) {
    initialize();
}

auto Waybar::initialize() -> void {
    m_outputs = getMonitorsInfo();
    
    // Initialize global workspace tracking
    g_current_workspace.store(getCurrentWorkspace(), std::memory_order_release);
    
    // Initialize waybar state - assume it starts visible
    m_waybar_visible = true;
    
    // Initialize mouse activation tracking
    m_mouse_in_activation_zone = false;
    
    // Only initialize config for modes that need it (focused and custom modes)
    if (m_original_mode == BarMode::HIDE_FOCUSED || m_original_mode == BarMode::HIDE_MON) {
        initConfig();
    }
}

Waybar::~Waybar() {
    // Ensure proper cleanup on destruction
    try {
        // Only restore config if it was loaded (focused and custom modes)
        if (m_original_mode == BarMode::HIDE_FOCUSED || m_original_mode == BarMode::HIDE_MON) {
            restoreOriginal();
        }
        reloadPid();
    } catch (const std::exception& e) {
        log_message(ERR, "Error during cleanup: {}", e.what());
    }
    cleanupSignals();
}

auto Waybar::run() -> void {
    switch (m_original_mode) {
    case BarMode::HIDE_FOCUSED: 
        runFocusedMode();
        break;
    
    case BarMode::HIDE_ALL: 
        hideAllMonitors();
        break;
        
    case BarMode::HIDE_MON: 
        runCustomMode();
        break;
    }
}

auto Waybar::runFocusedMode() -> void {
    log_message(INFO, "Launching Hide Focused Mode\n");
    hideFocused();
}

auto Waybar::runCustomMode() -> void {
    validateMonitorExists();
    hideCustom();
}

auto Waybar::validateMonitorExists() -> void {
    // Split comma-separated monitor names
    std::vector<std::string> monitor_names;
    std::stringstream ss(m_hidemon);
    std::string monitor;
    
    while (std::getline(ss, monitor, ',')) {
        // Trim whitespace
        monitor.erase(0, monitor.find_first_not_of(" \t"));
        monitor.erase(monitor.find_last_not_of(" \t") + 1);
        if (!monitor.empty()) {
            monitor_names.push_back(monitor);
        }
    }
    
    if (monitor_names.empty()) {
        log_message(CRIT, "No monitors specified after 'mon:'\n");
        throw std::invalid_argument("No monitors specified");
    }
    
    // Check if all specified monitors exist
    std::vector<std::string> missing_monitors;
    for (const auto& monitor_name : monitor_names) {
        bool exists = std::any_of(m_outputs.cbegin(), m_outputs.cend(), [&monitor_name](const monitor_info_t& m) {
            return m.name == monitor_name;
        });
        if (!exists) {
            missing_monitors.push_back(monitor_name);
        }
    }
    
    if (!missing_monitors.empty()) {
        log_message(CRIT, "Monitor(s) not found: ");
        for (const auto& missing : missing_monitors) {
            log_message(NONE, "'{}' ", missing);
        }
        log_message(NONE, "\n");
        log_message(NONE, "Available monitors: ");
        for (const auto& m : m_outputs)
            log_message(NONE, "{} ", m.name);
        log_message(NONE, "\n");
        throw std::invalid_argument("Monitor(s) not found");
    }
}

auto Waybar::hideCustom() -> void {
    if (m_outputs.size() <= Constants::SINGLE_MONITOR_THRESHOLD) {
        log_message(WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        hideAllMonitors();
        return;
    }

    setupCustomMode();
    runCustomModeLoop();
    cleanupCustomMode();
}

auto Waybar::setupCustomMode() -> void {
    // Parse comma-separated monitor names
    std::vector<std::string> target_monitors;
    std::stringstream ss(m_hidemon);
    std::string monitor;
    
    while (std::getline(ss, monitor, ',')) {
        // Trim whitespace
        monitor.erase(0, monitor.find_first_not_of(" \t"));
        monitor.erase(monitor.find_last_not_of(" \t") + 1);
        if (!monitor.empty()) {
            target_monitors.push_back(monitor);
        }
    }
    
    // filling output with all monitors except the target monitors
    Json::Value val(Json::arrayValue);
    for (auto& mon : m_outputs) {
        bool is_target = std::any_of(target_monitors.begin(), target_monitors.end(), [&mon](const std::string& target) {
            return mon.name == target;
        });
        
        if (!is_target) {
            val.append(mon.name);
        } else {
            mon.hidden = true;
        }
    }
    setOutputs(val);
    reloadPid();
    register_signals(this);
}

auto Waybar::runCustomModeLoop() -> void {
    auto [mouse_x, mouse_y] = initializeCustomModeMouse();

    while (!g_interrupt_request.load(std::memory_order_acquire)) {
        // Check for workspace changes
        if (checkWorkspaceChange()) {
            handleWorkspaceChange();
        }
        
        bool need_reload = processCustomModeIteration(mouse_x, mouse_y);
        requestApplyVisibleMonitors(need_reload);
        std::this_thread::sleep_for(Constants::POLLING_INTERVAL);
        std::tie(mouse_x, mouse_y) = getCursorPos();
    }
}

auto Waybar::initializeCustomModeMouse() -> std::pair<int, int> {
    return getCursorPos();
}

auto Waybar::processCustomModeIteration(int mouse_x, int mouse_y) -> bool {
    bool need_reload = false;
    
    // Parse comma-separated monitor names
    std::vector<std::string> target_monitors;
    std::stringstream ss(m_hidemon);
    std::string monitor;
    
    while (std::getline(ss, monitor, ',')) {
        // Trim whitespace
        monitor.erase(0, monitor.find_first_not_of(" \t"));
        monitor.erase(monitor.find_last_not_of(" \t") + 1);
        if (!monitor.empty()) {
            target_monitors.push_back(monitor);
        }
    }
    
    // Process each target monitor
    for (const auto& target_monitor : target_monitors) {
        auto& mon = getMonitor(target_monitor);
        const bool in_target_mon = is_cursor_in_monitor(mon, mouse_x, mouse_y);
        const int local_bar_threshold = mon.y_coord + m_bar_threshold;

        if (in_target_mon && !mon.hidden) {
            need_reload |= handleMonitorThreshold(mon, mouse_x, mouse_y, local_bar_threshold);
        } 
        else if (in_target_mon && mon.hidden && mouse_y < mon.y_coord + Constants::MOUSE_ACTIVATION_ZONE) {
            need_reload |= showHiddenMonitor(mon);
        }
    }

    return need_reload;
}

auto Waybar::showHiddenMonitor(monitor_info_t& mon) -> bool {
    if (m_verbose_level >= 1) {
        log_message(LOG, "Mon: {} needs to be shown.\n", mon.name);
    }
    mon.hidden = false;
    return true;
}

auto Waybar::cleanupCustomMode() -> void {
    log_message(LOG, "Restoring original config.\n");
    restoreOriginal();
    reloadPid();
    cleanupSignals();
}

auto Waybar::handleMonitorThreshold(monitor_info_t& mon, int& mouse_x, int& mouse_y, int local_bar_threshold) -> bool {
    if (mouse_y > local_bar_threshold) {
        if (m_verbose_level >= 1) {
            log_message(LOG, "Mon: {} needs to be hidden.\n", mon.name);
        }
        mon.hidden = true;
        return true;
    }
    
    // Keep showing while inside threshold
    while (mouse_y <= local_bar_threshold && !g_interrupt_request.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(Constants::POLLING_INTERVAL);
        std::tie(mouse_x, mouse_y) = getCursorPos();
    }
    
    if (m_verbose_level >= 1) {
        log_message(LOG, "Mon: {} needs to be hidden.\n", mon.name);
    }
    mon.hidden = true;
    return true;
}



auto Waybar::initConfig() -> void {
    m_config_path = findConfigPath();
    loadConfig();
    validateConfig();
}

auto Waybar::findConfigPath() -> std::string {
    auto path = getConfigPath();
    if (path.empty()) {
        throw std::runtime_error("Unable to find Waybar config");
    }
    log_message(INFO, "Waybar config file found in '{}'\n", path);
    return path;
}

auto Waybar::loadConfig() -> void {
    std::ifstream file(m_config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + m_config_path);
    }
    
    Json::CharReaderBuilder builder;
    std::string errors;
    if (!Json::parseFromStream(builder, file, &m_config, &errors)) {
        throw std::runtime_error("Invalid JSON in config file: " + errors);
    }
    
    m_backup = m_config;
    log_message(LOG, "Backuping original config.\n");
}

auto Waybar::validateConfig() -> void {
    if (m_config.isArray()) {
        log_message(CRIT, "Multiple bars are not supported.\n");
        throw std::runtime_error("Multiple bars are not supported");
    }
    if (!m_config.isMember("output")) {
        log_message(CRIT, "Config file does not contain 'output' field.\n");
        throw std::runtime_error("Config file does not contain 'output' field");
    }
}

auto Waybar::getConfigPath() -> std::string {
    auto str_cmd = get_process_args(m_waybar_pid);
    std::string_view cmd_view(str_cmd);
    auto config_pos = cmd_view.find("-c");
    
    if (config_pos != std::string_view::npos) {
        // Find the next argument after -c
        auto start = config_pos + 2;
        while (start < cmd_view.length() && std::isspace(cmd_view[start])) ++start;
        
        auto end = start;
        while (end < cmd_view.length() && !std::isspace(cmd_view[end])) ++end;
        
        if (start < end) {
            std::string config_path(cmd_view.substr(start, end - start));
            if (fs::exists(config_path)) {
                return config_path;
            }
        }
    }

    // Fallback to default config path
    std::string fallback_path = m_config_dir + "/config";
    if (fs::exists(fallback_path)) return fallback_path;
    
    return {};
}

auto Waybar::getOutputs() -> Json::Value& {
    return m_config["output"];
}

auto Waybar::setOutputs(const Json::Value &outputs) -> void {
    m_config["output"] = outputs;
    saveConfig();
}

auto Waybar::saveConfig() -> void {
    std::ofstream file(m_config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write config file: " + m_config_path);
    }
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(m_config, &file);
}

auto Waybar::restoreOriginal() -> void {
    std::ofstream file(m_config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write config file: " + m_config_path);
    }
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(m_backup, &file);
}

auto Waybar::hideAllMonitors(bool is_visible) -> void {
    setupAllMonitorsMode(is_visible);
    runAllMonitorsLoop(is_visible);
    cleanupAllMonitorsMode();
}

auto Waybar::setupAllMonitorsMode(bool& is_visible) -> void {
    if (is_visible) {
        hideWaybar();
        is_visible = false;
    }
    register_signals(this);
}

auto Waybar::cleanupAllMonitorsMode() -> void {
    reloadPid();
}

auto Waybar::runAllMonitorsLoop(bool is_visible) -> void {
    while (!g_interrupt_request.load(std::memory_order_acquire)) {
        auto [root_x, root_y] = getCursorPos();
        if (m_is_console and m_verbose_level >= 2)
            log_message(TRACE, "Mouse at position ({},{})\n", root_x, root_y);
        
        // Check for workspace changes
        if (checkWorkspaceChange()) {
            handleWorkspaceChange();
        }
        
        is_visible = processAllMonitorsVisibility(root_x, root_y, is_visible);
        std::this_thread::sleep_for(Constants::POLLING_INTERVAL);
    }
}

auto Waybar::processAllMonitorsVisibility(int /* root_x */, int root_y, bool is_visible) -> bool {
    for (auto &mon : m_outputs) {
        is_visible = processMonitorVisibility(mon, root_y, is_visible);
    }
    return is_visible;
}

auto Waybar::processMonitorVisibility(const monitor_info_t& mon, int root_y, bool is_visible) -> bool {
    const int local_bar_threshold = mon.y_coord + m_bar_threshold;
    
    if (!is_visible && shouldShowWaybar(mon, root_y)) {
        // Mouse is in activation zone - start or continue tracking
        if (!m_mouse_in_activation_zone) {
            m_mouse_in_activation_zone = true;
            m_mouse_activation_start = std::chrono::steady_clock::now();
        }
        
        // Check if mouse has been in activation zone long enough
        if (checkMouseActivationDelay()) {
            return showWaybarAndKeepOpen(mon, local_bar_threshold);
        }
    }
    else if (is_visible && shouldHideWaybar(mon, root_y, local_bar_threshold)) {
        return hideWaybarAndReturnFalse();
    }
    else {
        // Mouse is not in activation zone - reset tracking
        m_mouse_in_activation_zone = false;
    }
    
    return is_visible;
}

auto Waybar::showWaybarAndKeepOpen(const monitor_info_t& /* mon */, int local_bar_threshold) -> bool {
    showWaybar();
    auto [root_x, root_y] = getCursorPos();
    while (root_y < local_bar_threshold && !g_interrupt_request.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(Constants::POLLING_INTERVAL);
        std::tie(root_x, root_y) = getCursorPos();
    }
    return true;
}

auto Waybar::hideWaybarAndReturnFalse() -> bool {
    hideWaybar();
    return false;
}

auto Waybar::shouldShowWaybar(const monitor_info_t& mon, int root_y) const -> bool {
    return mon.y_coord <= root_y && root_y < mon.y_coord + Constants::MOUSE_ACTIVATION_ZONE;
}

auto Waybar::shouldHideWaybar(const monitor_info_t& mon, int root_y, int threshold) const -> bool {
    return root_y < mon.y_coord + mon.height && root_y > threshold;
}

auto Waybar::checkMouseActivationDelay() -> bool {
    if (!m_mouse_in_activation_zone) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - m_mouse_activation_start;
    
    return elapsed >= Constants::MOUSE_ACTIVATION_DELAY;
}

auto Waybar::showWaybar() -> void {
    if (!m_waybar_visible) {
        if (m_verbose_level >= 1) {
            log_message(LOG, "Opening it. \n");
        }
        if (kill(m_waybar_pid, SIGUSR1) == -1) {
            if (errno == ESRCH) {
                // Process doesn't exist, try to restart waybar
                log_message(WARN, "Waybar process {} not found, attempting restart...\n", m_waybar_pid);
                m_waybar_pid = restartWaybar();
                // Try again after restart
                if (kill(m_waybar_pid, SIGUSR1) == -1) {
                    throw std::runtime_error("Failed to send SIGUSR1 to restarted waybar process " + std::to_string(m_waybar_pid) + ": " + strerror(errno));
                }
            } else {
                throw std::runtime_error("Failed to send SIGUSR1 to waybar process " + std::to_string(m_waybar_pid) + ": " + strerror(errno));
            }
        }
        m_waybar_visible = true;
    }
}

auto Waybar::hideWaybar() -> void {
    if (m_waybar_visible) {
        if (m_verbose_level >= 1) {
            log_message(LOG, "Hiding it. \n");
        }
        if (kill(m_waybar_pid, SIGUSR1) == -1) {
            if (errno == ESRCH) {
                // Process doesn't exist, try to restart waybar
                log_message(WARN, "Waybar process {} not found, attempting restart...\n", m_waybar_pid);
                m_waybar_pid = restartWaybar();
                // Try again after restart
                if (kill(m_waybar_pid, SIGUSR1) == -1) {
                    throw std::runtime_error("Failed to send SIGUSR1 to restarted waybar process " + std::to_string(m_waybar_pid) + ": " + strerror(errno));
                }
            } else {
                throw std::runtime_error("Failed to send SIGUSR1 to waybar process " + std::to_string(m_waybar_pid) + ": " + strerror(errno));
            }
        }
        m_waybar_visible = false;
    }
}


auto Waybar::reloadPid() -> void {
    log_message(INFO, "Reloading PID: {}\n", m_waybar_pid);
    
    if (kill(m_waybar_pid, SIGUSR2) == -1) {
        if (errno == ESRCH) {
            // Process doesn't exist, try to restart waybar
            log_message(WARN, "Waybar process {} not found, attempting restart...\n", m_waybar_pid);
            m_waybar_pid = restartWaybar();
        } else {
            throw std::runtime_error("Failed to send SIGUSR2 to waybar process " + std::to_string(m_waybar_pid) + ": " + strerror(errno));
        }
    }
}


// Removed unnecessary helper function - logic inlined where needed

auto Waybar::hideFocused() -> void {
    if (m_outputs.size() <= Constants::SINGLE_MONITOR_THRESHOLD) {
        log_message(WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        hideAllMonitors();
        return;
    }

    setupFocusedMode();
    runFocusedModeLoop();
    cleanupFocusedMode();
}

auto Waybar::setupFocusedMode() -> void {
    validateFocusedModeConfig();
    register_signals(this);
    std::sort(m_outputs.begin(), m_outputs.end());
}

auto Waybar::validateFocusedModeConfig() -> void {
    if (auto &o = getOutputs();
        !o.isNull() && o.size() < m_outputs.size()) {
        log_message(LOG, "Some monitors are not in the Waybar config, adding all of them. \n");
        Json::Value val;
        for (const auto& mon : m_outputs)
            val.append(mon.name);
        setOutputs(val);
    }
}

auto Waybar::runFocusedModeLoop() -> void {
    auto [mouse_x, mouse_y] = initializeMousePosition();

    while (!g_interrupt_request.load(std::memory_order_acquire)) {
        // Check for workspace changes
        if (checkWorkspaceChange()) {
            handleWorkspaceChange();
        }
        
        bool need_reload = processFocusedMonitors(mouse_x, mouse_y);
        applyChanges(need_reload);
        sleepAndUpdateMouse(mouse_x, mouse_y);
    }
}

auto Waybar::initializeMousePosition() -> std::pair<int, int> {
    return getCursorPos();
}

auto Waybar::applyChanges(bool need_reload) -> void {
    requestApplyVisibleMonitors(need_reload);
}

auto Waybar::sleepAndUpdateMouse(int& mouse_x, int& mouse_y) -> void {
    std::this_thread::sleep_for(Constants::POLLING_INTERVAL);
    std::tie(mouse_x, mouse_y) = getCursorPos();
    if (m_is_console and m_verbose_level >= 2)
        log_message(TRACE, "Mouse at position ({},{})\n", mouse_x, mouse_y);
}

auto Waybar::processFocusedMonitors(int mouse_x, int mouse_y) -> bool {
    bool need_reload = false;

    for (auto& mon : m_outputs) {
        if (isCursorInCurrentMonitor(mon, mouse_x, mouse_y)) {
            need_reload |= processCurrentMonitor(mon, mouse_x, mouse_y);
        }
    }

    return need_reload;
}

auto Waybar::isCursorInCurrentMonitor(const monitor_info_t& mon, int mouse_x, int mouse_y) -> bool {
    return is_cursor_in_monitor(mon, mouse_x, mouse_y);
}

auto Waybar::processCurrentMonitor(monitor_info_t& mon, int mouse_x, int mouse_y) -> bool {
    if (!mon.hidden) {
        return handleVisibleMonitor(mon, mouse_x, mouse_y);
    } else {
        return handleHiddenMonitor(mon, mouse_x, mouse_y);
    }
}

auto Waybar::handleVisibleMonitor(monitor_info_t& mon, int mouse_x, int mouse_y) -> bool {
    const int local_bar_threshold = mon.y_coord + m_bar_threshold;
    return handleMonitorThreshold(mon, mouse_x, mouse_y, local_bar_threshold);
}

auto Waybar::handleHiddenMonitor(monitor_info_t& mon, int /* mouse_x */, int mouse_y) -> bool {
    if (mouse_y < mon.y_coord + Constants::MOUSE_ACTIVATION_ZONE) {
        if (m_verbose_level >= 1) {
            log_message(LOG, "Mon: {} needs to be shown.\n", mon.name);
        }
        mon.hidden = false;
        return true;
    }
    return false;
}

auto Waybar::cleanupFocusedMode() -> void {
    log_message(LOG, "Restoring original config.\n");
    restoreOriginal();
    reloadPid();
    cleanupSignals();
}

auto Waybar::getMonitor(const std::string &name) -> monitor_info_t& {
    for (auto& mon : m_outputs) {
        if (mon.name == name) {
            return mon;
        }
    }

    throw std::invalid_argument("Monitor '" + name + "' not found");
}

// Workspace monitoring functions
auto Waybar::getCurrentWorkspace() const -> int {
    std::string workspace_info = execute_command("hyprctl activeworkspace");
    if (workspace_info.empty()) {
        return 1; // fallback to workspace 1
    }
    
    // Parse workspace ID from output like "workspace ID 5 (5) on monitor HDMI-A-1:"
    std::istringstream iss(workspace_info);
    std::string token;
    while (iss >> token) {
        if (token == "ID") {
            iss >> token; // should be the workspace number
            try {
                return std::stoi(token);
            } catch (const std::exception&) {
                return 1; // fallback
            }
        }
    }
    
    return 1; // fallback
}

auto Waybar::checkWorkspaceChange() const -> bool {
    int current_workspace = getCurrentWorkspace();
    int previous_workspace = g_current_workspace.load(std::memory_order_acquire);
    
    if (current_workspace != previous_workspace) {
        g_current_workspace.store(current_workspace, std::memory_order_release);
        return true; // workspace changed
    }
    return false; // no change
}

auto Waybar::handleWorkspaceChange() -> void {
    int current_workspace = g_current_workspace.load(std::memory_order_acquire);
    if (m_verbose_level >= 1) {
        log_message(LOG, "Workspace changed to workspace {}\n", current_workspace);
    }
    showWaybarTemporarily();
}

auto Waybar::showWaybarTemporarily() -> void {
    if (m_verbose_level >= 1) {
        log_message(LOG, "Showing waybar temporarily for {} seconds after workspace change\n", Constants::WORKSPACE_SHOW_DURATION.count());
    }
    
    // Show waybar
    showWaybar();
    
    // Wait for the specified duration
    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < Constants::WORKSPACE_SHOW_DURATION) {
        if (g_interrupt_request.load(std::memory_order_acquire)) {
            break; // exit early if interrupted
        }
        std::this_thread::sleep_for(100ms);
    }
    
    // Hide waybar after duration
    hideWaybar();
    if (m_verbose_level >= 1) {
        log_message(LOG, "Waybar hidden after workspace change\n");
    }
}


