#include "utils.hpp"
#include "waybar.hpp"

auto main() -> int 
{

    if (std::string(std::getenv("XDG_CURRENT_DESKTOP")) == "Hyprland") {
        Waybar bar;
        bar.run(BarMode::HIDE_UNFOCUSED);
    }
    else {
        Utils::log(Utils::CRIT,"This tool only supports Hyprland WM. Exiting.\n");
    }

}
