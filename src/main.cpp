#include "waybar.hpp"
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <csignal>
#include <cstdlib>

auto getConfigDir() -> std::string {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home) + "/.config/waybar";
}

auto getPidFilePath() -> std::string {
    const char* xdg_runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (!xdg_runtime_dir) {
        throw std::runtime_error("XDG_RUNTIME_DIR environment variable not set");
    }
    return std::string(xdg_runtime_dir) + "/autowaybar.pid";
}

auto createPidFile() -> void {
    std::string pid_file = getPidFilePath();
    
    // Check if PID file already exists
    if (std::filesystem::exists(pid_file)) {
        // Read existing PID and check if process is still running
        std::ifstream file(pid_file);
        if (file.is_open()) {
            pid_t existing_pid;
            file >> existing_pid;
            file.close();
            
            // Check if process is still running
            if (kill(existing_pid, 0) == 0) {
                throw std::runtime_error("autowaybar is already running (PID: " + std::to_string(existing_pid) + ")");
            } else {
                // Process is dead, remove stale PID file
                std::filesystem::remove(pid_file);
            }
        }
    }
    
    // Create PID file
    std::ofstream file(pid_file);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create PID file: " + pid_file);
    }
    file << getpid() << std::endl;
    file.close();
}

auto removePidFile() -> void {
    std::string pid_file = getPidFilePath();
    std::filesystem::remove(pid_file);
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
            try {
                int threshold = std::stoi(std::string(optarg));
                if (threshold < Constants::MIN_THRESHOLD || threshold > Constants::MAX_THRESHOLD) {
                    log_message(CRIT, "Threshold must be between {} and {}\n", 
                               Constants::MIN_THRESHOLD, Constants::MAX_THRESHOLD);
                    printHelp();
                    exit(1);
                }
                args.threshold = threshold;
            } catch (const std::exception&) {
                log_message(CRIT, "Invalid threshold value: {}\n", optarg);
                printHelp();
                exit(1);
            }
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

// Global waybar instance for signal handling
static Waybar* g_waybar_instance = nullptr;

// Signal handler for cleanup
auto cleanup_handler(int signal) -> void {
    log_message(WARN, "Signal {} received, shutting down waybar...\n", signal);
    
    if (g_waybar_instance) {
        try {
            // Restore original waybar config and reload
            g_waybar_instance->restoreOriginal();
            g_waybar_instance->reloadPid();
        } catch (const std::exception& e) {
            log_message(ERR, "Error during waybar cleanup: {}\n", e.what());
        }
    }
    
    // Remove PID file
    removePidFile();
    
    // Exit cleanly
    std::exit(0);
}

auto main(int argc, char *argv[]) -> int {
    try {
        std::string config_dir = getConfigDir();
        Args args = parseArguments(argc, argv);

        if (args.help) {
            printHelp();
            return 0;
        }

        // Create PID file to prevent multiple instances
        createPidFile();
        
        // Set up signal handlers for cleanup
        std::signal(SIGINT, cleanup_handler);
        std::signal(SIGTERM, cleanup_handler);
        std::signal(SIGHUP, cleanup_handler);
        
        // Ensure cleanup on exit
        std::atexit([]() { removePidFile(); });
        
        Waybar bar(args.mode, args.threshold, args.verbose, config_dir);
        g_waybar_instance = &bar;  // Set global pointer for signal handler
        bar.run();
        
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        log_message(CRIT, "Error: {}\n", e.what());
        removePidFile(); // Clean up PID file on error
        return 1;
    }
}

