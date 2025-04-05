#include "waybar.hpp"
#include <getopt.h>

auto main(int argc, char *argv[]) -> int {
    const char *short_opts = "m:ht:";

    const struct option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {"threshold", required_argument, nullptr, 't'},
        {nullptr, 0, nullptr, 0}
    };

    std::string mode{};
    int opt = 0;
    int threshold = 50;
    bool helpFlag = false;

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

    // Init waybar
    Waybar bar(mode, threshold);
    bar.run();
    
    return EXIT_SUCCESS;
}

