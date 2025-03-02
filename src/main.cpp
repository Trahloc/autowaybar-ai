#include "utils.hpp"
#include "waybar.hpp"
#include <getopt.h>

auto printHelp() -> void {
    using namespace fmt;

    struct Flag {
        std::string_view name;
        std::string_view description;
    };

    print(fg(color::yellow) | emphasis::bold, "autowaybar: \n");
    print(fg(color::cyan), "Program to manage visibility modes for waybar in Hyprland\n\n");
    print(fg(color::yellow) | emphasis::bold, "Usage:\n");

    print(fg(color::cyan), "  autowaybar ");
    print(fg(color::magenta) | emphasis::bold, "-m");
    print(fg(color::cyan), "/");
    print(fg(color::magenta) | emphasis::bold, "--mode ");
    print(fg(color::white), "<Mode> \n");

    constexpr std::array<Flag, 3> flags = {{
        {.name = "-m --mode", .description = "Select the operation mode for waybar."},
        {.name = "-t --threshold", .description = "Threshold in pixels that should match your waybar width"},
        {.name = "-h --help", .description = "Show this help"}
    }};

    size_t maxFlagLength = 0;
    for (const auto& flag : flags) maxFlagLength = std::max(maxFlagLength, flag.name.length());

    print(fg(color::yellow) | emphasis::bold, "Flags:\n");
    for (const auto& flag : flags) {
        print(fg(color::magenta) | emphasis::bold, "  {:<{}}", flag.name, maxFlagLength + 2);
        print("  {}\n", flag.description);
    }

    // examples
    print("\n");
    print(fg(color::yellow) | emphasis::bold, "Examples:\n");
    print(fg(color::cyan), "  autowaybar -m focused\n");
    print(fg(color::cyan), "  autowaybar -m all\n");
    //print(fg(color::cyan), "  autowaybar -m mon:<monitorname>\n");
    print(fg(color::cyan), "  autowaybar -m focused -t 100\n");
    print(fg(color::cyan), "  autowaybar -m all -t 100\n");

    // Detailed mode descriptions
    print(fg(color::yellow) | emphasis::bold, "\nMode:\n");
    print(fg(color::cyan), "  focused: ");
    print(emphasis::italic, "Hide the focused monitor and show the rest. When the mouse reaches the top,\n"
    "  it will show the current monitor, same as `all` mode. (If only 1 monitor is active, it will fallback to `all` mode.)\n\n");
    print(fg(color::cyan), "  all: ");
    print(emphasis::italic, "Hide all monitors, when the mouse reaches the top of the screen, \n"
    "  both will be shown and when you go down the `threshold`, they will be hidden again.\n\n");
    //print(fg(color::cyan), "  mon:<monitorname>: ");
    //print(emphasis::italic, "Hide the bar only on the specified monitor.\n\n");
}

auto main(int argc, char *argv[]) -> int {
    const char *short_opts = "m:ht:";

    const struct option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {"threshold", required_argument, nullptr, 't'},
        {nullptr, 0, nullptr, 0}
    };

    std::string mode{};
    int opt, threshold = -1;
    bool helpFlag = false;
    BarMode auxMode = BarMode::NONE;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
        case 'm': mode = optarg; break;
        case 'h': helpFlag = true; break;
        case 't': threshold = std::stoi(std::string(optarg)); break;
        default: printHelp(); return 1;
        }
    }

    if (helpFlag) {
        printHelp();
        return 0;
    }

    // Parse mode argument
    if (mode.empty()) {
        Utils::log(Utils::CRIT, "-m / --mode is mandatory.\n");
        printHelp();
        return EXIT_FAILURE;
    } else if (mode == "all") {
        auxMode = BarMode::HIDE_ALL;
    } else if (mode == "focused") {
        auxMode = BarMode::HIDE_FOCUSED;
    } else {
        Utils::log(Utils::CRIT, "Invalid mode value: {}\n", mode);
        printHelp();
        return EXIT_FAILURE;
    }

    // Init waybar
    Waybar bar = (threshold != -1) ?
            Waybar(auxMode, threshold) :
            Waybar(auxMode);
    bar.run();
    
    return EXIT_SUCCESS;
}

