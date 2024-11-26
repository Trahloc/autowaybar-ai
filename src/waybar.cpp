#include "waybar.hpp"
#include <iostream>
#include <thread>

Waybar::Waybar() {
    outputs = Utils::Hyprland::getMonitorsInfo();
    full_config = getCurrentML4WConfig();
    Utils::log(Utils::INFO, "Waybar config found in: {}", full_config.string());
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
        std::cout << "[+] current theme: " << config_name << std::endl;

        // get the stuff
        fs::path config_file { ml4w_config_root / config_name / "config" };
        if (fs::is_regular_file(config_file)) {

            std::cout << "[+] Full config file found in: " << config_file.string() << std::endl;
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

        std::cout << "[+] Found mouse at position at: " << root_x << ", " << root_y << "\n";

        // show waybar
        if (!open && root_y < 5) {
            std::cout << "[+] opening it\n";
            //system(toggle_hide.c_str());
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
            std::cout << "[+] It should die \n";
            //system(toggle_hide.c_str());
            kill(WAYBAR_PID, SIGUSR1);
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }
}