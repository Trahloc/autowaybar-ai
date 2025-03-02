#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <json/value.h>
#include <stdexcept>
#include <thread>
#include "utils.hpp"
#include "waybar.hpp"

// Exclusive for Hyprland, wont work with other WM
namespace Hyprland {

    // returns cursor x and y coords
    auto getCursorPos() -> std::pair<int, int> {
        const std::string cmd = "hyprctl cursorpos";
        std::istringstream stream(Utils::execCommand(cmd));

        int xpos, ypos;
        char basurilla;
        if (stream >> xpos >> basurilla >> ypos)
            return std::pair<int, int>{xpos, ypos};

        return std::pair<int, int>{-1, -1};
    }

    // returns a vector with the monitor information provided by Hyprland
    auto getMonitorsInfo() -> std::vector<monitor_info_t> {
        const std::string cmd = "hyprctl monitors all -j";
        std::istringstream stream(Utils::execCommand(cmd));

        Json::Value data;
        stream >> data;

        std::vector<monitor_info_t> monitors;
        monitors.reserve(2);

        // fetch all monitors info
        for (int i = 0; i < data.size(); ++i) {
            monitor_info_t temp;

            // names
            if (!data[i]["name"].empty()) {
                temp.name = data[i]["name"].asString();
            }

            // x coord
            if (!data[i]["x"].empty()) {
                temp.x_coord = data[i]["x"].asInt();
            }

            if (!data[i]["width"].empty()) {
                temp.width = data[i]["width"].asInt();
            }

            Utils::log(Utils::LogLevel::LOG, "Monitor named {} found in x: {}, width: {}. \n", temp.name, temp.x_coord, temp.width);
            monitors.push_back(temp);
        }

        return monitors;
    }

} // namespace Hyprland

auto Waybar::initPid() const -> pid_t {
    try {
        return std::stoi(Utils::execCommand("pidof waybar"));
    } catch(std::exception& e) {
        Utils::log(Utils::CRIT, "Waybar is not running.\n");
        std::exit(EXIT_FAILURE);
    }
}


Waybar::Waybar(BarMode mode)
    : m_original_mode(mode),
      m_config_path(initConfigPath()),
      m_outputs(Hyprland::getMonitorsInfo()),
      m_waybar_pid(initPid()) {
    if (std::string(std::getenv("XDG_CURRENT_DESKTOP")) != "Hyprland") {
        Utils::log(Utils::CRIT, "This tool ONLY supports Hyprland.");
        std::exit(EXIT_FAILURE);
    }
}

Waybar::Waybar(BarMode mode, int threshold)
    : Waybar(mode) {
    m_bar_threshold = threshold;
}

// returns the default config file if found
auto Waybar::initFallBackConfig() const  -> fs::path {
    for (auto path : g_possible_config_lookup) {
        if (fs::exists(path) || 
            fs::exists(path.replace_extension(fs::path{".jsonc"})))
            return path;
    }
    return {};
}

auto Waybar::run() -> void {
    if (m_original_mode == BarMode::HIDE_FOCUSED) {
        if (m_config_path.empty()) {
            m_config_path = initFallBackConfig();
            if (m_config_path.empty())
                throw std::runtime_error("Unable to find Waybar config.\n");
        }

        Utils::log(Utils::INFO, "Waybar config file found in '{}'\n", m_config_path.string());
        Utils::log(Utils::INFO, "Launching Hide Focused Mode\n");

        hideFocused();
    }
    else if (m_original_mode == BarMode::HIDE_ALL) {
        hideAllMonitors();
    }
}

auto Waybar::initConfigPath() const  -> fs::path {
    auto str_cmd = Utils::getProcArgs(m_waybar_pid);

    auto find = str_cmd.find("-c");
    if (find != std::string::npos) {
        str_cmd.erase(str_cmd.begin(), str_cmd.begin() + find + 2);
        find = str_cmd.find("-s");

        if (find != std::string::npos) {
            str_cmd.erase(str_cmd.begin() + find, str_cmd.end());

            // strip special characters
            str_cmd.erase(
                std::remove(str_cmd.begin(), str_cmd.end(), '\000'),
                str_cmd.end()
            );

            return str_cmd;
        }
    }

    return {};
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

        Utils::log(Utils::LOG, "Found mouse at position ({},{})\n", root_x, root_y);

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
    std::ifstream file(m_config_path);

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
    file.close();

    if (config.isArray()) {
        Utils::log(Utils::CRIT, "Multiple bars are not supported.\n");
        std::exit(EXIT_FAILURE);
    }

    // filling output with all monitors in case some are missing
    if (!config["output"].isNull() && config["output"].size() < m_outputs.size()) {
        Utils::log(Utils::INFO, "Some monitors are not in the Waybar config, adding all of them. \n");
        Json::Value val;
        for (const auto& mon : m_outputs)
            val.append(mon.name);
        config["output"] = val;
    }

    // store output with all monitors in case of exiting
    const Json::Value initial_outputs = config["output"];

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // fall back option when only 1 monitor
    if (m_outputs.size() <= 1) {
        Utils::log(Utils::WARN, "The number of monitors is {}. Fall back to `mode` ALL\n", m_outputs.size());
        hideAllMonitors();
    }

    // create an overwriteable ofstream
    std::ofstream o_file(m_config_path);

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
            config["output"] = getVisibleMonitors();
            Utils::log(Utils::LOG, "New update: {}", fmt::streamed(config["output"]));
            Utils::truncateFile(o_file, m_config_path); // We delete all the file
            o_file << config;
            o_file.close();
            reload();
        }

        // wait and update mouse
        std::this_thread::sleep_for(80ms);
        std::tie(mouse_x, mouse_y) = Hyprland::getCursorPos();
        Utils::log(Utils::INFO, "Mouse ({},{})\n", mouse_x, mouse_y);
    }

    // restore original config
    config["output"] = initial_outputs;
    Utils::truncateFile(o_file, m_config_path);
    o_file << config;
    o_file.close();
    reload();
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
