#include "Hyprland.hpp"

// Exclusive for Hyprland, wont work with other WM
namespace Hyprland {

    // returns cursor x and y coords
    auto getCursorPos() -> std::pair<int, int> {
        const std::string_view cmd = "hyprctl cursorpos";
        std::istringstream stream(Utils::execCommand(cmd));

        int xpos, ypos;
        char basurilla;
        if (stream >> xpos >> basurilla >> ypos)
            return std::pair<int, int>{xpos, ypos};

        return std::pair<int, int>{-1, -1};
    }

    // returns a vector with the monitor information provided by Hyprland
    auto getMonitorsInfo() -> std::vector<monitor_info_t> {
        const std::string_view cmd = "hyprctl monitors all -j";
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

            float scale = 1.0f;
            if (!data[i]["scale"].empty()) {
               scale = data[i]["scale"].asFloat();
            }

            // calculate width taking into account scaling factor
            if (!data[i]["width"].empty()) {
                temp.width = data[i]["width"].asInt() / scale;
            }

            Utils::log(Utils::LogLevel::LOG, "Monitor named {} found in x: {}, width: {}. \n", temp.name, temp.x_coord, temp.width);
            monitors.push_back(temp);
        }

        return monitors;
    }

} // namespace Hyprland