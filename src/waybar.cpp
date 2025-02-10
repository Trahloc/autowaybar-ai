#include "waybar.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <json/value.h>
#include <stdexcept>
#include <thread>

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

Waybar::Waybar() {
    try {
        m_waybar_pid = std::stoi(Utils::execCommand("pidof waybar"));
    } catch(std::exception& e) {
        Utils::log(Utils::CRIT, "Waybar is not running.\n");
        std::exit(0);
    }

}

auto Waybar::run(BarMode mode) -> void {
    if (std::string(std::getenv("XDG_CURRENT_DESKTOP")) == "Hyprland") {
        if (mode == BarMode::HIDE_UNFOCUSED) {

            m_outputs = Hyprland::getMonitorsInfo();
            auto [m_config_path, m_style_path] = getConfigAndStyle();

            if (!m_config_path.empty() && !m_style_path.empty()) {
                Utils::log(Utils::INFO, "Waybar config found in: {}\n", m_config_path.string());
                Utils::log(Utils::INFO, "Waybar style found in: {}\n", m_style_path.string());
            } else {
                /* fallbackConfigAndStyle(); */
            }

            hideUnfocused();
        }
        else if (mode == BarMode::HIDE_ALL) {
            hideAllMonitors();
        }
    }
    else {
        Utils::log(Utils::CRIT, "This tool ONLY supports Hyprland.");
    }
}

auto Waybar::getConfigAndStyle() -> std::pair<fs::path, fs::path> {

    auto proc_command = Utils::getProcArgs(m_waybar_pid);

    auto find = proc_command.find("-c");
    if (find != std::string::npos) {
       proc_command.erase(proc_command.begin(), proc_command.begin() + find);
       std::cout << "PITO: " << proc_command;
    }


    return {};
}

// WARNING: For now, it will ONLY suport ML4W dotfiles
/* auto Waybar::getCurrentML4WConfig() -> fs::path {
    const fs::path ml4w_config_root = g_HOMEDIR / ".config/waybar/themes";
    std::string current_config;

    // find current config name based on ML4W lookup script locations
    for (const auto& path : possible_config_lookup) {
        if (fs::exists(path)) {
            current_config = Utils::execCommand("cat " + path.string());
            break;
        }
    }

    if (!current_config.empty()) {
        // parse current config
        int delimiter = current_config.find(";");
        auto config_name = current_config.substr(1, delimiter - 1);
        Utils::log(Utils::INFO, "Current theme: {} \n", config_name);

        // get the stuff
        fs::path config_file { ml4w_config_root / config_name / "config" };
        if (fs::is_regular_file(config_file)) {
            Utils::log(Utils::INFO, "Full config file found in: {} \n", config_file.string());
            return config_file;
        }

    }

    return {};
} */


auto Waybar::hideAllMonitors() -> void {
    bool open = false;

    kill(m_waybar_pid, SIGUSR1);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

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
            while (temp.second < m_bar_threshold) {
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

auto Waybar::reload() -> void {
    Utils::log(Utils::INFO, "Reloading PID: {}\n", m_waybar_pid);
    kill(m_waybar_pid, SIGUSR2);
}

auto Waybar::hideUnfocused() -> void {
    // read initial config
    std::ifstream file(m_config_path);
    if (!file) throw std::runtime_error("[CRIT] Couldn't open config file.\n");

    Json::Value config;
    file >> config;
    file.close();

    if (config.isArray()) {
        // std::cerr << "[CRIT] Multiple bars are not supported.\n";
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

    // easiest start: only if we have more than 1 monitor
    if (m_outputs.size() > 1) {
        // create an overwriteable ofstream
        std::ofstream o_file(m_config_path);

        // sort monitors ASCENDING based on x starting position
        std::sort(m_outputs.begin(), m_outputs.end() );
        auto [x, y] = Hyprland::getCursorPos();

        while (!g_interruptRequest) {
            Json::Value temp = Json::arrayValue;
            bool need_reload = false; // this is false, until otherwise
            std::string hidden_name;

            for (auto& mon : m_outputs) {
                if (mon.x_coord + mon.width < x && mon.hidden){
                    // si estas a mi izquierda oculto -> mostrar
                    Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                    mon.hidden = false;
                    need_reload = true;
                }
                else if (mon.x_coord < x && mon.x_coord + mon.width > x && !mon.hidden) {
                    // si estas en mi monitor y no estas oculto -> hide
                    Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                    mon.hidden = true;
                    hidden_name = mon.name;
                    need_reload = true;
                    break;
                }
                else if (mon.x_coord > x && mon.hidden) {
                    // si esta a la derecha y oculto -> mostrar
                    Utils::log(Utils::INFO, "Mon: {} needs to be shown.\n", mon.name);
                    mon.hidden = false;
                    need_reload = true;
                }

            }

            // add the rest to show them
            for (const auto& mon: m_outputs) {
                if (!mon.hidden && mon.name != hidden_name) {
                    temp.append(mon.name);
                }
            }

            // only update in something changed
            if (need_reload && !temp.isNull()) {
                Utils::log(Utils::LOG, "Updating\n");
                config["output"] = temp;
                std::cout << "New update: " << config["output"] << "\n";
                Utils::truncateFile(o_file, m_config_path); // We delete all the file
                o_file << config;
                o_file.close();
                reload();
            }

            // wait and update mouse
            std::this_thread::sleep_for(80ms);
            std::tie(x,y) = Hyprland::getCursorPos();
            Utils::log(Utils::INFO, "Mouse ({},{})\n", x,y);
        }

        // restore original config
        config["output"] = initial_outputs;
        Utils::truncateFile(o_file, m_config_path);
        o_file << config;
        o_file.close();
        reload();
    }
    // unhandled case
    else {
        Utils::log(Utils::WARN, "This feature requieres a minimum of 2 monitors.\n");
        return;
    }
}
