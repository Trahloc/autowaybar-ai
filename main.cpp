#include <iostream>
#include <filesystem>
#include <array>
#include <thread>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

constexpr int THRESHOLD = 43;

const std::array<fs::path, 2> possible_config_lookup = {
    "$HOME/.config/ml4w/settings/waybar-theme.sh",
    "$HOME/.cache/.themestyle.sh"
};

// returns cursor pos by a hyprland command
auto getCursorPos() -> std::pair<int, int> {

    FILE* pipe = popen("hyprctl cursorpos", "r");

    if (!pipe) {
        std::cerr << "Pipe failed to exec hyprctl command";
        return {-1, -1};
    }

    char buffer[128];
    std::string result;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    std::istringstream stream(result);
    int xpos, ypos;
    char basurilla;

    if (stream >> xpos >> basurilla >> ypos)
        return {xpos, ypos};


    return {-1, -1};
}

auto main() -> int {

    bool open = false;
    std::string command_toggle = "killall -SIGUSR1 waybar";

    while (true) {
        auto [root_x, root_y] = getCursorPos();

        std::cout << "[+] Found mouse at position at: " << root_x << ", " << root_y << "\n";

        // show waybar
        if (!open && root_y < 5) {
            std::cout << "[+] opening it\n";
            system(command_toggle.c_str());
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
            system(command_toggle.c_str());
            open = false;
        }

        std::this_thread::sleep_for(80ms);
    }
}
