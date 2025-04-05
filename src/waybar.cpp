#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <json/value.h>
#include <stdexcept>
#include <thread>
#include "utils.hpp"
#include "Hyprland.hpp"

auto Waybar::initPid() const -> pid_t {
    try {
        return std::stoi(Utils::execCommand("pidof waybar"));
    } catch(std::exception& e) {
        Utils::log(Utils::CRIT, "Waybar is not running.\n");
        std::exit(EXIT_FAILURE);
    }
}

// Parse mode argument
auto Waybar::parseMode(std::string mode) -> BarMode {
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


Waybar::Waybar(std::string mode, int threshold)
    : m_waybar_pid(initPid()),
      m_is_console(isatty(fileno(stdin))),
      m_bar_threshold(threshold) {
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

        // hiding logic
        throw std::runtime_error("Unimplemented");
        m_config->init();
        break;
    }
    default:
        Utils::log(Utils::CRIT, "Uknown mode.");
    }
}

auto Waybar::hideAllMonitors() const -> void {
    bool open = false;
    kill(m_waybar_pid, SIGUSR1);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // hide bar in both monitors
    while (!g_interruptRequest) {
        auto [root_x, root_y] = Hyprland::getCursorPos();

	    // show mouse position only if it runs in terminal -> eg. stop trashing all the log files
        if (m_is_console)
            Utils::log(Utils::LOG, "Mouse at position ({},{})\n", root_x, root_y);

        // show waybar
        if (!open && root_y < 5) {
            Utils::log(Utils::INFO, "Opening it. \n");
            kill(m_waybar_pid, SIGUSR1);
            open = true;

            auto temp = Hyprland::getCursorPos();

            // keep it open
            while (temp.second < m_bar_threshold && !g_interruptRequest) {
                std::this_thread::sleep_for(80ms);
                temp = Hyprland::getCursorPos();
            }

        }
        // closing waybar
        else if (open && root_y > m_bar_threshold) {
            Utils::log(Utils::INFO, "Hiding it. \n");
            kill(m_waybar_pid, SIGUSR1);
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }

    // Restoring initial config in case of interruption
    reload();
}

auto Waybar::reload() const -> void {
    Utils::log(Utils::INFO, "Reloading PID: {}\n", m_waybar_pid);
    kill(m_waybar_pid, SIGUSR2);
}

auto Waybar::hideFocused() -> void {
    // read initial config
    /* std::ifstream file(m_config_path);
    const bool isConsole = isatty(fileno(stdin));

    if (!file) throw std::runtime_error("[CRIT] Couldn't open config file.\n");
    
    Json::Value config;
    try { 
        file >> config; 
    } 
    catch (std::exception e) { 
        Utils::log(Utils::CRIT, "Invalid waybar json file at {}", m_config_path.string()); 
        file.close();
        std::exit(EXIT_FAILURE);
    }
    file.close(); */

    // back up config
    // Utils::log(Utils::LOG, "Backuping original config.\n");
    // const Json::Value backup_config = config; 

    // if (config.isArray()) {
    //     Utils::log(Utils::CRIT, "Multiple bars are not supported.\n");
    //     std::exit(EXIT_FAILURE);
    // }

    // filling output with all monitors in case some are missing
    if (auto &o = m_config->getOutputs();
        !o.isNull() && o.size() < m_outputs.size()) {
        Utils::log(Utils::LOG, "Some monitors are not in the Waybar config, adding all of them. \n");
        Json::Value val;
        for (const auto& mon : m_outputs)
            val.append(mon.name);
        m_config->setOutputs(val);
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // fall back option when only 1 monitor
    if (m_outputs.size() <= 1) {
        Utils::log(Utils::WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        m_config->restoreOriginal();
        reload();
        hideAllMonitors();
    }

    // create an overwriteable ofstream
    // std::ofstream o_file(m_config_path);

    // sort monitors ASCENDING based on x starting position
    std::sort(m_outputs.begin(), m_outputs.end() );
    auto [mouse_x, mouse_y] = Hyprland::getCursorPos();

    // main loop
    while (!g_interruptRequest) {
        bool need_reload {false}; // this is false, until otherwise

        for (auto& mon : m_outputs) {
            const bool in_current_mon = mon.x_coord < mouse_x && mon.x_coord + mon.width > mouse_x;

            // if current monitor shown
            if (in_current_mon && !mon.hidden) {
                // outside of the threshold -> hide it 
                if (mouse_y > m_bar_threshold) {
                    Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                    mon.hidden = true;
                    need_reload = true;
                } 
                // inside the threshold -> keep showing
                else if (mouse_y <= m_bar_threshold) {
                    while (!g_interruptRequest && mouse_y <= m_bar_threshold) {
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
            else if (in_current_mon && mon.hidden && mouse_y < 5) {
                Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                mon.hidden = false;
                need_reload = true;
            }
            // if left/right hidden -> show it
            else if ((mon.x_coord + mon.width < mouse_x || mon.x_coord > mouse_x) && mon.hidden) {
                Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                mon.hidden = false;
                need_reload = true;
            }
        }

        // only update in something changed
        if (need_reload) {
            Utils::log(Utils::LOG, "Updating\n");
            m_config->setOutputs(getVisibleMonitors());
            Utils::log(Utils::LOG, "New update: {}", fmt::streamed(m_config->getOutputs()));
            reload(); // always reload to apply changes
        }

        // wait and update mouse
        std::this_thread::sleep_for(80ms);
        std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
        if (m_is_console) 
            Utils::log(Utils::INFO, "Mouse at position ({},{})\n", mouse_x, mouse_y);
    }

    Utils::log(Utils::LOG, "Restoring original config.\n");
    m_config->restoreOriginal();
    reload(); // apply
}

inline auto Waybar::getVisibleMonitors() const -> Json::Value {
    Json::Value arr(Json::arrayValue);
    for (const auto& mon : m_outputs) {
        if (!mon.hidden) {
            arr.append(mon.name);
        }
    }
    return arr;
}
