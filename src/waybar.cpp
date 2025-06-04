#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <json/value.h>
#include <stdexcept>
#include <sys/types.h>
#include <thread>
#include "utils.hpp"
#include "Hyprland.hpp"

// Auxiliary functions

auto register_signals(__sighandler_t handler) -> void {
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
    std::signal(SIGHUP, handler);
}

auto is_cursor_in_monitor(const monitor_info_t &mon, int x, int y) -> bool {
    return mon.x_coord <= x and
       mon.x_coord + mon.width >= x and
       mon.y_coord <= y and
       mon.y_coord + mon.height >= y;
}

// Waybar functions

// reloads waybar with the new visible monitors
auto Waybar::requestApplyVisibleMonitors(bool need_reload) -> void {
    if (need_reload) {
        Utils::log(Utils::LOG, "Updating\n");
        m_config->setOutputs(getVisibleMonitors());
        Utils::log(Utils::LOG, "New update: {}", fmt::streamed(m_config->getOutputs()));
        reloadPid(); // always reload to apply changes
    }
}

auto Waybar::logMousePos(int x, int y) const -> void {
    if (m_is_console and m_is_verbose)
        Utils::log(Utils::INFO, "Mouse at position ({},{})\n", x, y);
}
 
auto Waybar::initPid() const -> pid_t {
    try {
        return std::stoi(Utils::execCommand("pidof waybar"));
    } catch(std::exception& e) {
        Utils::log(Utils::CRIT, "Waybar is not running.\n");
        std::exit(EXIT_FAILURE);
    }
}

// Parse mode argument
auto Waybar::parseMode(const std::string &mode) -> BarMode {
    BarMode res = BarMode::NONE;
    
    if (mode.empty()) {
        Utils::log(Utils::CRIT, "-m / --mode is mandatory.\n");
        printHelp();
        std::exit(EXIT_FAILURE);
    }

    if (mode == "all") 
        res = BarMode::HIDE_ALL;
    else if (mode == "focused") 
        res = BarMode::HIDE_FOCUSED;
    else if (mode.length() > 4 && mode.substr(0, 4) == "mon:") {
        m_hidemon = mode.substr(4); 
        res = BarMode::HIDE_MON;
    }
    else {
        Utils::log(Utils::CRIT, "Invalid mode value: {}\n", mode);
        printHelp();
        std::exit(EXIT_FAILURE);
    }
    return res;
}


Waybar::Waybar(const std::string &mode, int threshold, bool verbose)
    : m_waybar_pid(initPid()),
      m_is_console(isatty(fileno(stdin))),
      m_bar_threshold(threshold),
      m_is_verbose(verbose) {
    if (std::string(std::getenv("XDG_CURRENT_DESKTOP")) != "Hyprland") {
        Utils::log(Utils::CRIT, "This tool ONLY supports Hyprland.");
        std::exit(EXIT_FAILURE);
    }
    m_original_mode = parseMode(mode);
    m_outputs = Hyprland::getMonitorsInfo();
    m_config = std::make_unique<config>(m_waybar_pid);
}

auto Waybar::run() -> void {
    switch (m_original_mode) {
    case BarMode::HIDE_FOCUSED: 
        m_config->init();
        Utils::log(Utils::INFO, "Launching Hide Focused Mode\n");
        hideFocused();
        break;
    
    case BarMode::HIDE_ALL: 
        hideAllMonitors();
        break;
        
    case BarMode::HIDE_MON: {
        // check if hide mon exist
        bool exist = std::any_of(m_outputs.cbegin(), m_outputs.cend(), [this](monitor_info_t m) {
            return m.name == m_hidemon;
        });

        if (!exist) {
            Utils::log(Utils::CRIT, "Monitor '{}' provided, was not found.\n", m_hidemon);
            Utils::log(Utils::NONE, "Consider using any of this: ");
            for (const auto& m : m_outputs)
                Utils::log(Utils::NONE, "{} ", m.name);
            Utils::log(Utils::NONE, "\n");
            std::exit(EXIT_FAILURE);
        }

        m_config->init();
        hideCustom();
        break;
    }
    case BarMode::NONE:
        Utils::log(Utils::INFO, "Doing nothing\n");
    }
}

auto Waybar::hideCustom() -> void {
    // fall back option when only 1 monitor
    if (m_outputs.size() <= 1) {
        Utils::log(Utils::WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        hideAllMonitors();
        return;
    }

    // filling output with all monitors except the target
    Json::Value val(Json::arrayValue);
    for (auto& mon : m_outputs) {
        if (mon.name != m_hidemon) val.append(mon.name);
        else mon.hidden = true;
    }
    m_config->setOutputs(val);
    reloadPid();

    register_signals(handleSignal);

    auto [mouse_x, mouse_y] = Hyprland::getCursorPos();
    auto &mon = getMonitor(m_hidemon); 
    const int local_bar_threshold = mon.y_coord + m_bar_threshold;

    // main loop
    while (!g_interruptRequest) {
        bool need_reload {false}; // this is false, until otherwise
        const bool in_target_mon = is_cursor_in_monitor(mon, mouse_x, mouse_y);

        // if target monitor shown
        if (in_target_mon && !mon.hidden) {
            // outside of the threshold -> hide it 
            if (mouse_y > local_bar_threshold) {
                Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                mon.hidden = true;
                need_reload = true;
            } 
            // inside the threshold -> keep showing
            else if (mouse_y <= local_bar_threshold) {
                while (!g_interruptRequest && mouse_y <= local_bar_threshold) {
                    std::this_thread::sleep_for(80ms);
                    std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
                }
                // we escaped the threshold -> hide it 
                Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                mon.hidden = true;
                need_reload = true;
            }
        } 
        // if we touch top of the monitor -> show it 
        else if (in_target_mon && mon.hidden && mouse_y < mon.y_coord + 7) {
            Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
            mon.hidden = false;
            need_reload = true;
        }
    
        // only update in something changed
        requestApplyVisibleMonitors(need_reload);

        // wait and update mouse
        std::this_thread::sleep_for(80ms);
        std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
        logMousePos(mouse_x, mouse_y);
    }

    Utils::log(Utils::LOG, "Restoring original config.\n");
    m_config->restoreOriginal();
    reloadPid(); // apply
}

auto Waybar::hideAllMonitors(bool is_visible) const -> void {
    if (is_visible) {
        kill(m_waybar_pid, SIGUSR1);
        is_visible = false;
    }

    register_signals(handleSignal);

    // hide bar in both monitors
    while (!g_interruptRequest) {
        auto [root_x, root_y] = Hyprland::getCursorPos();
        logMousePos(root_x, root_y);

        for (auto &mon : m_outputs) {
            const int local_bar_threshold = mon.y_coord + m_bar_threshold;
            
            // show waybar
            if (!is_visible && mon.y_coord <= root_y && root_y < mon.y_coord + 7) {
                Utils::log(Utils::INFO, "Opening it. \n");
                kill(m_waybar_pid, SIGUSR1);
                is_visible = true;
                std::tie(root_x, root_y) = Hyprland::getCursorPos();

                // keep it open
                while (root_y < local_bar_threshold && !g_interruptRequest) {
                    std::this_thread::sleep_for(80ms);
                    std::tie(root_x, root_y) = Hyprland::getCursorPos();
                }
            }
            // closing waybar
            else if (is_visible &&
                     root_y < mon.y_coord + mon.height &&
                     root_y > local_bar_threshold) {
                Utils::log(Utils::INFO, "Hiding it. \n");
                kill(m_waybar_pid, SIGUSR1);
                is_visible = false;
            }
        }
        std::this_thread::sleep_for(80ms);
    }
    // Restoring initial config in case of interruption
    reloadPid();
}

auto Waybar::reloadPid() const -> void {
    Utils::log(Utils::INFO, "Reloading PID: {}\n", m_waybar_pid);
    kill(m_waybar_pid, SIGUSR2);
}

auto Waybar::hideFocused() -> void {
    // fall back option when only 1 monitor
    if (m_outputs.size() <= 1) {
        Utils::log(Utils::WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        hideAllMonitors();
        return;
    }

    // filling output with all monitors in case some are missing
    if (auto &o = m_config->getOutputs();
        !o.isNull() && o.size() < m_outputs.size()) {
        Utils::log(Utils::LOG, "Some monitors are not in the Waybar config, adding all of them. \n");
        Json::Value val;
        for (const auto& mon : m_outputs)
            val.append(mon.name);
        m_config->setOutputs(val);
    }

    register_signals(handleSignal);

    // sort monitors ASCENDING based on x starting position
    std::sort(m_outputs.begin(), m_outputs.end() );
    auto [mouse_x, mouse_y] = Hyprland::getCursorPos();

    // main loop
    while (!g_interruptRequest) {
        bool need_reload {false}; // this is false, until otherwise

        for (auto& mon : m_outputs) {
            const bool in_current_mon = is_cursor_in_monitor(mon, mouse_x, mouse_y);
            const int local_bar_threshold = mon.y_coord + m_bar_threshold;

            // if current monitor shown
            if (in_current_mon && !mon.hidden) {
                // outside of the threshold -> hide it 
                if (mouse_y > local_bar_threshold) {
                    Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                    mon.hidden = true;
                    need_reload = true;
                } 
                // inside the threshold -> keep showing
                else if (mouse_y <= local_bar_threshold) {
                    while (!g_interruptRequest && mouse_y <= local_bar_threshold) {
                        std::this_thread::sleep_for(80ms);
                        std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
                    }
                    // we scaped the threshold -> hide it 
                    Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                    mon.hidden = true;
                    need_reload = true;
                }
            } 
            // if we touch top -> show it 
            else if (in_current_mon && mon.hidden && mouse_y < mon.y_coord + 7) {
                Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                mon.hidden = false;
                need_reload = true;
            }
            // if left/right and top/bottom hidden -> show it
            else if (((mon.x_coord + mon.width < mouse_x || mon.x_coord > mouse_x) || (mon.y_coord + mon.height < mouse_y || mon.y_coord > mouse_y)) && mon.hidden) {
                Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                mon.hidden = false;
                need_reload = true;
            }
        }

        // only update in something changed
        requestApplyVisibleMonitors(need_reload);

        // wait and update mouse
        std::this_thread::sleep_for(80ms);
        std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
        logMousePos(mouse_x, mouse_y);
    }

    Utils::log(Utils::LOG, "Restoring original config.\n");
    m_config->restoreOriginal();
    reloadPid(); // apply
}

auto Waybar::getMonitor(const std::string &name) -> monitor_info_t& {
    for (auto& mon : m_outputs)
        if (mon.name == name)
            return mon;

    throw std::runtime_error("Monitor " + name + " not found.\n");
}

inline auto Waybar::getVisibleMonitors() const -> Json::Value {
    Json::Value arr(Json::arrayValue);
    for (const auto& mon : m_outputs) {
        if (!mon.hidden)
            arr.append(mon.name);
    }
    return arr;
}
