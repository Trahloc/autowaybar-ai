#include "utils.hpp"

auto Utils::execCommand(const std::string& command) -> std::string {
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

// returns cursor x and y coords
auto Utils::Hyprland::getCursorPos() -> std::pair<int, int> {
    const std::string cmd = "hyprctl cursorpos";
    std::istringstream stream(Utils::execCommand(cmd));

    int xpos, ypos;
    char basurilla;
    if (stream >> xpos >> basurilla >> ypos)
        return {xpos, ypos};

    return {-1, -1};
}


auto Utils::Hyprland::getMonitorsInfo() -> std::vector<monitor_info> {
    const std::string cmd = "hyprctl monitors -j";
    std::istringstream stream(Utils::execCommand(cmd));

    Json::Value data;
    stream >> data;
    
    std::vector<monitor_info> monitors(2);

    // fetch all monitors info
    for (int i = 0; i < data.size(); ++i) {
        monitor_info temp;

        // names
        if (!data[i]["name"].empty()) {
            temp.name = data[i]["name"].asString();
            Utils::log(LOG, "Monitor named {} found. \n", temp.name);
        }
        // width
        if (int width = data[i]["width"].asInt(); width) {
            temp.width = width;
        }
        // x coord
        if (int x_coord = data[i]["x"].asInt(); x_coord) {
            temp.x_coord = x_coord;
        }

        monitors.push_back(temp);
    }

    return monitors;
}