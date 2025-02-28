#include "utils.hpp"
#include "waybar.hpp"
#include <cstdlib>
#include <getopt.h>

auto printHelp() -> void {
    static const std::string help {
        "Usage: \n" 
        "autowaybar -m | --mode <MODE>\n" 
        "   MODE: all, focused\n" 
        "       - all: Will hide all monitors, when the mouse reaches the top of the screen, both will be shown and when\n"
        "              you go down the `threshold`, they will be hidden again.\n"
        "       - focused: Will always hide the focused monitor and show the rest.\n"
        "              When the mouse reaches the top, it will show the current monitor,\n"
        "              same as `all` mode. (If only 1 monitor is active, it will fallback\n"
        "              to `all` mode.)\n"
        "autowaybar -h | --help\n" 
        "   Prints this help\n"
        "autowaybar -t | --threshold <px> \n"
        "   Changes the amount of pixels of height autowaybar will keep showing the bar.\n"
        "   Set it to a higher number if your bar is more thick\n"
        "   (Default: 50px) \n"
    };
    Utils::log(Utils::INFO, help); 
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
            Waybar( auxMode, threshold) :
            Waybar(auxMode);
    bar.run();
    
    return EXIT_SUCCESS;
}

