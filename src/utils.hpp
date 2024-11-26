#pragma once 

#include <fmt/base.h>
#include <fmt/format.h>
#include <json/json.h>
#include <string>
#include <fmt/core.h>
#include <vector>

struct monitor_info {
    std::string name;
    int x_coord, width;
};

namespace Utils {

    auto execCommand(const std::string& command) -> std::string;

    enum LogLevel {
        NONE = -1,
        LOG  = 0,
        WARN,
        ERR,
        CRIT,
        INFO,
        TRACE
    };

    template <typename... Args>
    void log(LogLevel level, const std::string &fmt, Args&&... args); 

    // Exclusive for Hyprland, wont work with other WM
    namespace Hyprland {

        auto getCursorPos() -> std::pair<int, int>; // returns cursor x and y coords
        auto getMonitorsInfo() -> std::vector<monitor_info>;

    } // namespace Hyprland

} // namespace Utils
