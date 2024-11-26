#include "waybar.hpp"

// hides the unfocused monitor only
/*
auto hideUnfocusedMonitor(screen_info_t monitors) -> void {
    fs::path waybar_config = getCurrentML4WConfig();

    if (waybar_config.empty()) {
        std::cout << "[+] Waybar config file couldn't be found. Aborting.  \n";
        return;
    }

    std::ifstream file(waybar_config);
    Json::Value config;
    file >> config;

}
*/

auto main() -> int {

    Waybar bar;
    bar.run(BarMode::HIDE_ALL);

}
