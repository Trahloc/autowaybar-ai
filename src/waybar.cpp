#include "waybar.hpp"
#include "utils.hpp"
#include <climits>
#include <iostream>
#include <thread>

Waybar::Waybar() {
    outputs = Utils::Hyprland::getMonitorsInfo();
    full_config = getCurrentML4WConfig();
    Utils::log(Utils::INFO, "Waybar config found in: {}\n", full_config.string());
}

auto Waybar::run(BarMode mode) -> void {
    if (mode == BarMode::HIDE_ALL) {
        hideAllMonitors();
    } 
    else if (mode == BarMode::HIDE_UNFOCUSED) {
        ;
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
        kill(WAYBAR_PID, SIGUSR1);
    }

    // hide bar in both monitors
    while (true) {
        auto [root_x, root_y] = Utils::Hyprland::getCursorPos();

        Utils::log(Utils::LOG, "Found mouse at position ({},{})\n", root_x, root_y);

        // show waybar
        if (!open && root_y < 5) {
            Utils::log(Utils::INFO, "Opening it. \n");
            kill(WAYBAR_PID, SIGUSR1);
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
            kill(WAYBAR_PID, SIGUSR1);
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }
}