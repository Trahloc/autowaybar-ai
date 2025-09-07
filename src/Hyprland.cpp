#include "Hyprland.hpp"

// Exclusive for Hyprland, wont work with other WM

// returns cursor x and y coords
auto getCursorPos() -> std::pair<int, int> {
    const std::string_view cmd = "hyprctl cursorpos";
    std::string result = execute_command(cmd);
    
    if (result.empty()) {
        return std::pair<int, int>{-1, -1};
    }
    
    std::istringstream stream(result);
    int xpos, ypos;
    char separator;
    
    if (stream >> xpos >> separator >> ypos) {
        return std::pair<int, int>{xpos, ypos};
    }

    return std::pair<int, int>{-1, -1};
}

// returns a vector with the monitor information provided by Hyprland
auto getMonitorsInfo() -> std::vector<monitor_info_t> {
        const std::string_view cmd = "hyprctl monitors all -j";
        std::istringstream stream(execute_command(cmd));

        Json::Value data;
        Json::CharReaderBuilder builder;
        std::string errors;
        
        if (!Json::parseFromStream(builder, stream, &data, &errors)) {
            throw std::runtime_error("Invalid JSON response from hyprctl");
        }

        if (!data.isArray()) {
            throw std::runtime_error("Invalid JSON structure from hyprctl");
        }

        std::vector<monitor_info_t> monitors;
        monitors.reserve(data.size());

        for (const auto& monitor : data) {
            monitor_info_t temp;
            temp.name = monitor["name"].asString();
            temp.x_coord = monitor["x"].asInt();
            temp.y_coord = monitor["y"].asInt();
            
            float scale = monitor["scale"].empty() ? 1.0f : monitor["scale"].asFloat();
            temp.width = static_cast<int>(monitor["width"].asInt() / scale);
            temp.height = static_cast<int>(monitor["height"].asInt() / scale);

            log_message(LOG,
                "Monitor named {} found in x: {}, y: {}, width: {}, height: {}. \n",
                temp.name, temp.x_coord, temp.y_coord, temp.width, temp.height
            );
            monitors.push_back(temp);
        }

        return monitors;
    }
