#include "waybar.hpp"

// Exclusive functions for Hyprland, wont work with other compositor
auto getCursorPos() -> std::pair<int, int>;
auto getMonitorsInfo() -> std::vector<monitor_info_t>;
