#include "utils.hpp"
#include "waybar.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cstdio>

// Get process arguments by PID
auto get_process_args(const pid_t pid) -> std::string {
    std::string proc_path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(proc_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot read process arguments");
    }
    
    std::string info;
    std::getline(file, info);
    return info;
}

// Execute command and return stdout
auto execute_command(const std::string_view command) -> std::string {
    std::string cmd_str(command);
    if (cmd_str.empty()) return {};
    
    FILE* pipe = popen(cmd_str.c_str(), "r");
    if (!pipe) return {};
    
    constexpr size_t BUFFER_SIZE = 128;
    char buffer[BUFFER_SIZE];
    std::string result;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}
