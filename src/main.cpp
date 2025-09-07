#include "waybar.hpp"
#include <getopt.h>

auto main(int argc, char *argv[]) -> int {
    const char *short_opts = "m:ht:v";

    const struct option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {"threshold", required_argument, nullptr, 't'},
        {"verbose", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0}
    };

    std::string mode{};
    int opt = 0;
    int threshold = Constants::DEFAULT_BAR_THRESHOLD;
    bool helpFlag = false;
    bool verbose = false;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
        case 'm': {
            if (optarg && strlen(optarg) > 0) {
                mode = optarg;
            } else {
                Utils::log(Utils::ERR, "Mode parameter cannot be empty\n");
                printHelp();
                return 1;
            }
            break;
        }
        case 'h': helpFlag = true; break;
        case 't': {
            try {
                threshold = std::stoi(std::string(optarg));
                if (threshold < 1 || threshold > 1000) {
                    Utils::log(Utils::ERR, "Threshold must be between 1 and 1000 pixels, got: {}\n", threshold);
                    printHelp();
                    return 1;
                }
            } catch (const std::exception& e) {
                Utils::log(Utils::ERR, "Invalid threshold value: {}\n", optarg);
                printHelp();
                return 1;
            }
            break;
        }
        case 'v': verbose = true; break;
        default: printHelp(); return 1;
        }
    }

    if (helpFlag) {
        printHelp();
        return 0;
    }

    // Init waybar
    Waybar bar(mode, threshold, verbose);
    bar.run();
    
    return EXIT_SUCCESS;
}

