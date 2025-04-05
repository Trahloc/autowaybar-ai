#include "utils.hpp"
#include <fstream>
#include <istream>
#include <stdexcept>
#include <string>
#include <unistd.h>

/*
    @param pid : the pid of the process to get its arguments
    @returns : a string containing the arguments
*/
auto Utils::getProcArgs(const pid_t pid) -> std::string {
    std::string pid_s = std::to_string(pid);
    std::ifstream proc_args("/proc/" + pid_s + "/cmdline");

    if (proc_args.is_open()) {
        std::string info;
        std::getline(proc_args, info);
        return info;
    } else
        throw std::runtime_error("Invalid PID: " + pid_s);
}


/*
    @param command : string containing the program to be executed
    @returns : a string containing the stdout from the executed program
*/
auto Utils::execCommand(const std::string_view command) -> std::string {
    FILE* pipe = popen(command.data(), "r");

    if (!pipe) {
        Utils::log(Utils::ERR, "Failed to execute command: {}", command);
        return {};
    }

    char buffer[128];
    std::string result;

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    pclose(pipe);

    return result;
}
