#include "Hyprland.hpp"

// Exclusive for Hyprland, wont work with other WM

// Check if we're running in Hyprland - fail fast if not
auto isHyprlandRunning() -> bool {
    const char* session = std::getenv("XDG_SESSION_DESKTOP");
    return session && std::string(session) == "Hyprland";
}

// returns cursor x and y coords
auto getCursorPos() -> std::pair<int, int> {
    if (!isHyprlandRunning()) {
        const char* session = std::getenv("XDG_SESSION_DESKTOP");
        std::string session_str = session ? session : "unknown";
        throw std::runtime_error("This tool only works with Hyprland. Current session: " + session_str);
    }
    
    const std::string_view cmd = "/usr/bin/hyprctl cursorpos";
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
    if (!isHyprlandRunning()) {
        const char* session = std::getenv("XDG_SESSION_DESKTOP");
        std::string session_str = session ? session : "unknown";
        throw std::runtime_error("This tool only works with Hyprland. Current session: " + session_str);
    }
    
    const std::string_view cmd = "/usr/bin/hyprctl monitors all -j";
    std::string result = execute_command(cmd);
    
    if (result.empty()) {
        throw std::runtime_error("Failed to get monitor information from hyprctl");
    }
    
    std::istringstream stream(result);
    Json::Value data;
    Json::CharReaderBuilder builder;
    std::string errors;
    
    if (!Json::parseFromStream(builder, stream, &data, &errors)) {
        throw std::runtime_error("Invalid JSON response from hyprctl: " + errors);
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
