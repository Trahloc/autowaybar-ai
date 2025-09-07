#include "utils.hpp"
#include "raii_wrappers.hpp"
#include "waybar.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

/*
    @param pid : the pid of the process to get its arguments
    @returns : a string containing the arguments
*/
auto Utils::getProcArgs(const pid_t pid) -> std::string {
    std::string pid_s = std::to_string(pid);
    std::string proc_path = "/proc/" + pid_s + "/cmdline";
    
    try {
        FileWrapper proc_args(proc_path);
        std::string info;
        std::getline(proc_args.get(), info);
        return info;
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid PID: " + pid_s + " - " + e.what());
    }
}


/*
    @param command : string containing the program to be executed
    @returns : a string containing the stdout from the executed program
*/
auto Utils::execCommand(const std::string_view command) -> std::string {
    // Comprehensive input validation to prevent command injection
    std::string cmd_str(command);
    
    // Check for empty or whitespace-only commands
    if (cmd_str.empty() || cmd_str.find_first_not_of(" \t\n\r") == std::string::npos) {
        Utils::log(Utils::ERR, "Empty or whitespace-only command provided");
        return {};
    }
    
    // Check for dangerous characters that could lead to command injection
    if (cmd_str.find_first_of(";&|`$(){}[]<>\"'\\") != std::string::npos) {
        Utils::log(Utils::ERR, "Invalid characters in command: {}", command);
        return {};
    }
    
    // Check for suspicious patterns
    if (cmd_str.find("..") != std::string::npos || 
        cmd_str.find("//") != std::string::npos ||
        cmd_str.find("&&") != std::string::npos ||
        cmd_str.find("||") != std::string::npos) {
        Utils::log(Utils::ERR, "Suspicious pattern detected in command: {}", command);
        return {};
    }
    
    // Validate command length (prevent extremely long commands)
    if (cmd_str.length() > 1024) {
        Utils::log(Utils::ERR, "Command too long: {} characters", cmd_str.length());
        return {};
    }
    
    try {
        ProcessPipe pipe(cmd_str);
        
        char buffer[Constants::COMMAND_BUFFER_SIZE];
        std::string result;

        while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result += buffer;
        }

        return result;
    } catch (const std::exception& e) {
        Utils::log(Utils::ERR, "Failed to execute command '{}': {}", command, e.what());
        return {};
    }
}
