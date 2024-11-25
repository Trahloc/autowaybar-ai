#include <cstdlib>
#include <iostream>
#include <json/json.h>
#include <filesystem>
#include <json/value.h>
#include <string>
#include <thread>
#include <vector>
#include <array>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

constexpr  int THRESHOLD     = 43;
const fs::path HOMEDIR       = std::string(std::getenv("HOME"));
const fs::path WAYBAR_BASE_CONFIG = HOMEDIR / ".config/waybar/themes";

const std::array<fs::path, 2> possible_config_lookup = {
    HOMEDIR / ".config/ml4w/settings/waybar-theme.sh",
    HOMEDIR / ".cache/.themestyle.sh"
};

// creates a pipe, executes de command and returns output as a string
auto execCommand(const std::string& command) -> std::string {
    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe) {
        std::cerr << "[ERROR] Failed to execute command: " << command << ".\n";
        return {};
    }

    char buffer[128];
    std::string result;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    return result;
}

// returns a vector with the available monitor names
auto getMonitors() -> std::vector<std::string> {
    const std::string cmd = "hyprctl monitors -j";
    std::istringstream stream(execCommand(cmd));

    Json::Value data;
    stream >> data;
    
    std::vector<std::string> result;
    result.reserve(2);

    for (int i = 0; i < data.size(); ++i) {
        if (!data[i]["name"].empty())
            result.push_back(data[i]["name"].asString());
    }

    return result;
}

// returns cursor x and y coords
auto getCursorPos() -> std::pair<int, int> {
    const std::string cmd = "hyprctl cursorpos";
    std::istringstream stream(execCommand(cmd));

    int xpos, ypos;
    char basurilla;
    if (stream >> xpos >> basurilla >> ypos)
        return {xpos, ypos};

    return {-1, -1};
}

// hides waybar in both monitors at the same time
auto hideAllMonitors() -> void {
    bool open = false;
    const std::string toggle_hide = "killall -SIGUSR1 waybar";

    // If its initialy visible, hide it first
    std::cout << "Is waybar visible? [Y/N]: ";
    char c = std::cin.get();
    if (c == 'Y' || c == 'y') {
        system(toggle_hide.c_str());
    }

    // hide bar in both monitors
    while (true) {
        auto [root_x, root_y] = getCursorPos();

        std::cout << "[+] Found mouse at position at: " << root_x << ", " << root_y << "\n";

        // show waybar
        if (!open && root_y < 5) {
            std::cout << "[+] opening it\n";
            system(toggle_hide.c_str());
            open = true;

            auto temp = getCursorPos();

            // keep it open
            while (temp.second < THRESHOLD) {
                std::this_thread::sleep_for(80ms);
                temp = getCursorPos();
            }

        }
        // closing waybar
        else if (open && root_y > THRESHOLD) {
            std::cout << "[+] It should die \n";
            system(toggle_hide.c_str());
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }
}

// only hides the unfocused monitor
// WARNING: For now, it will ONLY suport ML4W dotfiles
auto getCurrentML4WConfig() -> fs::path {

    std::string current_config;

    // find current config name based on ML4W lookup script locations
    for (auto& path : possible_config_lookup) {
        if (fs::exists(path)) {
            current_config = execCommand("cat " + path.string());
            break;
        }
    }

    if (!current_config.empty()) {
        
        // parse current config
        int delimiter = current_config.find(";");
        auto config_name = current_config.substr(1, delimiter - 1);
        std::cout << "[+] current theme: " << config_name << std::endl;

        // get the stuff
        fs::path config_file { WAYBAR_BASE_CONFIG / config_name / "config" };
        if (fs::is_regular_file(config_file)) {

            std::cout << "[+] Full config file found in: " << config_file.string() << std::endl;
            return config_file;
        }

    }

    return {};
}

auto hideUnfocusedMonitor() -> void {

    auto waybar_config = getCurrentML4WConfig();

    if (waybar_config.empty()) {
        std::cout << "[+] Waybar config file couldn't be found. Aborting.  \n";
        return;
    }


}

auto main() -> int {

    const auto monitors = getMonitors();

    std::cout << "Available monitors: ";
    for (auto& mon : monitors)
        std::cout << mon << ", ";
    std::cout << std::endl;

    // run 
    //hideAllMonitors();
    hideUnfocusedMonitor();

}
