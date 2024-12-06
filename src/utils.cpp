#include "utils.hpp"
#include <istream>
#include <stdexcept>
#include <unistd.h>

auto Utils::truncateFile(std::ofstream& file, const fs::path& filepath) -> void {
    file.close();
    file.open(filepath, std::iostream::trunc);
    if (!file.is_open())
        throw std::runtime_error("[ERR] Couldn't open the file.\n");
}

auto Utils::execCommand(const std::string& command) -> std::string {
    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe) {
        std::cerr << "[ERR] Failed to execute command: " << command << ".\n";
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
        return std::pair<int, int>{xpos, ypos};

    return std::pair<int, int>{-1, -1};
}


auto Utils::Hyprland::getMonitorsInfo() -> std::vector<monitor_info> {
    const std::string cmd = "hyprctl monitors -j";
    std::istringstream stream(Utils::execCommand(cmd));

    Json::Value data;
    stream >> data;
    
    std::vector<monitor_info> monitors;
    monitors.reserve(2);

    // fetch all monitors info
    for (int i = 0; i < data.size(); ++i) {
        monitor_info temp;

        // names
        if (!data[i]["name"].empty()) {
            temp.name = data[i]["name"].asString();
        }
        
        // x coord
        if (!data[i]["x"].empty()) {
            temp.x_coord = data[i]["x"].asInt();
        }

        if (!data[i]["width"].empty()) {
            temp.width = data[i]["width"].asInt();
        }

        Utils::log(LOG, "Monitor named {} found in x: {}, width: {}. \n", temp.name, temp.x_coord, temp.width);
        monitors.push_back(temp);
    }

    return monitors;
}