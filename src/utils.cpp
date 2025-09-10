#include "utils.hpp"
#include "waybar.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <cstdio>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <fcntl.h>

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
    if (command.empty()) return {};
    
    // Split command into arguments for execv
    std::vector<std::string> args;
    std::stringstream ss;
    ss << command;
    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }
    
    if (args.empty()) return {};
    
    // Create argv array for execv
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    
    // Use pipe and fork for safe execution
    int pipefd[2];
    if (pipe(pipefd) == -1) return {};
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        // Redirect stderr to /dev/null to avoid mixing with stdout
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        close(pipefd[1]);
        
        execv(argv[0], argv.data());
        std::exit(1);
    } else if (pid > 0) {
        // Parent process
        close(pipefd[1]);
        
        constexpr size_t BUFFER_SIZE = 128;
        char buffer[BUFFER_SIZE];
        std::string result;
        
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            result += buffer;
        }
        
        close(pipefd[0]);
        waitpid(pid, nullptr, 0);
        return result;
    }
    
    return {};
}
