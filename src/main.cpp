#include <getopt.h>
#include "waybar.hpp"
#include <unistd.h>

auto printHelp() -> void {
    std::cout << "Usage: \n" << 
        "autowaybar -m <MODE>\n" <<
        "   MODE= all, unfocused\n" << 
        "autowaybar -h | --help\n" << 
        "   Prints this help\n";
}

int main(int argc, char *argv[]) {
    const char *short_opts = "m:h";

    const struct option long_opts[] = {
        {"mode", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    std::string mode;
    int opt;
    bool helpFlag = false;

    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
        case 'm':
            mode = optarg;
            break;
        case 'h':
            helpFlag = true;
            break;
        default:
            printHelp();
            return 1;
        }
    }

    if (helpFlag) {
        printHelp();
        return 0;
    }

    if (mode.empty()) {
        std::cerr << "Error: -m / --mode is mandatory.\n";
        printHelp();
        return 1;
    }

    if (mode != "all" && mode != "unfocused") {
        std::cerr << "Error: Invalid mode value. Allowed values are 'all' or 'unfocused'.\n";
        printHelp();
        return 1;
    }

    {
        Waybar bar;
        bar.run((mode == "all") ? BarMode::HIDE_ALL : BarMode::HIDE_UNFOCUSED);
    }

    return 0;
}

