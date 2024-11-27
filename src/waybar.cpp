#include "waybar.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <json/value.h>
#include <stdexcept>
#include <thread>

Waybar::Waybar() {

    try {
        waybar_pid = std::stoi(Utils::execCommand("pidof waybar"));
    } catch(std::exception& e) {
        Utils::log(Utils::CRIT, "Waybar is not running.\n");
        std::exit(0);
    }

    outputs = Utils::Hyprland::getMonitorsInfo();
    full_config = getCurrentML4WConfig();
    Utils::log(Utils::INFO, "Waybar config found in: {}\n", full_config.string());
}

auto Waybar::run(BarMode mode) -> void {
    
    if (mode == BarMode::HIDE_ALL) {
        hideAllMonitors();
    } 
    else if (mode == BarMode::HIDE_UNFOCUSED) {
        hideUnfocused();
    }

}

// WARNING: For now, it will ONLY suport ML4W dotfiles
auto Waybar::getCurrentML4WConfig() -> fs::path {

    const fs::path ml4w_config_root = HOMEDIR / ".config/waybar/themes";
    std::string current_config;

    // find current config name based on ML4W lookup script locations
    for (auto& path : possible_config_lookup) {
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
}


auto Waybar::hideAllMonitors() -> void {
    bool open = false;

    // If its initialy visible, hide it first
    std::cout << "Is waybar visible? [Y/N]: ";
    char c = std::cin.get();
    if (c == 'Y' || c == 'y') {
        //system(toggle_hide.c_str());
        kill(waybar_pid, SIGUSR1);
    }

    // hide bar in both monitors
    while (true) {
        auto [root_x, root_y] = Utils::Hyprland::getCursorPos();

        Utils::log(Utils::LOG, "Found mouse at position ({},{})\n", root_x, root_y);

        // show waybar
        if (!open && root_y < 5) {
            Utils::log(Utils::INFO, "Opening it. \n");
            kill(waybar_pid, SIGUSR1);
            open = true;

            auto temp = Utils::Hyprland::getCursorPos();

            // keep it open
            while (temp.second < bar_threshold) {
                std::this_thread::sleep_for(80ms);
                temp = Utils::Hyprland::getCursorPos();
            }

        }
        // closing waybar
        else if (open && root_y > bar_threshold) {
            Utils::log(Utils::INFO, "Hiding it. \n");
            kill(waybar_pid, SIGUSR1);
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }
}

auto Waybar::reload() -> void {
    kill(waybar_pid, SIGUSR2);
}

auto Waybar::hideUnfocused() -> void {
    // read initial config
    std::ifstream file(full_config);
    if (!file.is_open()) {
        throw std::runtime_error("[CRIT] Couldn't open config file.\n");
    }
    Json::Value config;
    file >> config;
    file.close();

    // filling output with all monitors in case some are missing
    if (!config["output"].isNull() && config["output"].size() < outputs.size()) {
        Utils::log(Utils::INFO, "Some monitors are not in the Waybar config, adding all of them. \n");
        Json::Value val;
        for (const auto& mon : outputs)
            val.append(mon.name);
        config["output"] = val;
    }

    // store output with all monitors in case of exiting
    const Json::Value initial_outputs = config["output"];

    std::signal(SIGINT, handleSignal);

    // easiest start: only if we have more than 1 monitor
    if (config["output"].size() > 1) {
        
        // create an overwriteable ofstream
        std::ofstream o_file(full_config, std::ofstream::trunc);

        // sort monitors DESCENDING based on x starting position
        std::sort(outputs.begin(), outputs.end() );
        std::reverse(outputs.begin(), outputs.end());

        auto [x, y] = Utils::Hyprland::getCursorPos();
        bool need_reload = false;

        while (!interruptRequest) {
            Json::Value temp;

            for (auto& mon : outputs) {
                
                // needs to be hidden
                if (mon.x_coord < x && !mon.hidden){
                    Utils::log(Utils::INFO, "Mon: {} needs to be hidden.\n", mon.name);
                    mon.hidden = true;
                    need_reload = true;
                    continue;
                }
                // we should show this one
                else {
                    Utils::log(Utils::INFO, "Mon: {} its okay.\n", mon.name);
                    temp.append(mon.name);
                    mon.hidden = false;
                }
            }

            // only update in something changed
            if (need_reload) {
                std::cout << "UPDATING.\n";
                config["output"] = temp;
                std::cout << "New update: " << config["output"] << "\n";
                Utils::truncateFile(o_file, full_config); // We delete all the file
                o_file << config;
                reload();
                need_reload = false;
            }
            std::this_thread::sleep_for(500ms);
            std::tie(x,y) = Utils::Hyprland::getCursorPos();
            Utils::log(Utils::INFO, "Mouse ({},{})\n", x,y);
        }

        // restore original config
        config["output"] = initial_outputs;
        Utils::truncateFile(o_file, full_config);
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

