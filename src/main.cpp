#include "waybar.hpp"
#include <getopt.h>

auto getConfigDir() -> std::string {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home) + "/.config/waybar";
}

struct Args {
    std::string mode{};
    int threshold = Constants::DEFAULT_BAR_THRESHOLD;
    bool help = false;
    int verbose = 0;  // 0 = normal, 1 = -v (LOG), 2 = -vv (TRACE)
};

auto parseArguments(int argc, char* argv[]) -> Args {
    const char *short_opts = "m:ht:v";
    const struct option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {"threshold", required_argument, nullptr, 't'},
        {"verbose", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0}
    };

    Args args;
    int opt = 0;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
        case 'm':
            args.mode = optarg;
            break;
        case 'h':
            args.help = true;
            break;
        case 't':
            args.threshold = std::stoi(std::string(optarg));
            break;
        case 'v':
            args.verbose++;
            break;
        default:
            printHelp();
            exit(1);
        }
    }

    return args;
}

auto main(int argc, char *argv[]) -> int {
    try {
        std::string config_dir = getConfigDir();
        Args args = parseArguments(argc, argv);

        if (args.help) {
            printHelp();
            return 0;
        }

        Waybar bar(args.mode, args.threshold, args.verbose, config_dir);
        bar.run();
        
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        log_message(CRIT, "Error: {}\n", e.what());
        return 1;
    }
}

