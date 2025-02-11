#pragma once

#include <csignal>
#include <fstream>
#include <iostream>
#include <fmt/base.h>
#include <fmt/format.h>
#include <json/json.h>
#include <string>
#include <fmt/core.h>
#include <filesystem>
#include "colors.h"

namespace fs = std::filesystem;

namespace Utils {
    enum LogLevel {
        NONE = -1,
        LOG  = 0,
        WARN,
        ERR,
        CRIT,
        INFO,
        TRACE
    };

    auto getProcArgs(const pid_t pid) -> std::string;
    auto execCommand(const std::string& command) -> std::string;
    auto truncateFile(std::ofstream& file, const fs::path& filepath) -> void;

    template <typename... Args>
    auto log(Utils::LogLevel level, const std::string &fmt, Args&&... args) -> void {
        switch (level) {
                case NONE: break;
                case LOG: std::cout << "[" << GRAY << "LOG" << RESET << "] "; break;
                case WARN: std::cout << "[" << YELLOW << "WARN" << RESET << "] "; break;
                case ERR: std::cout << "[" << ORANGE << "ERR" << RESET << "] "; break;
                case CRIT: std::cout << "[" << RED << "CRIT" << RESET << "] "; break;
                case INFO: std::cout << "[" << BLUE << "INFO" << RESET << "] "; break;
                case TRACE: std::cout << "[" << GRAY << "TRACE" << RESET << "] "; break;
            }
            fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
    }

} // namespace Utils
