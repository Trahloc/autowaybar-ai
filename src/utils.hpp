#pragma once

#include <csignal>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/color.h>
#include <json/json.h>
#include <string>

enum LogLevel {
    NONE = -1,
    LOG  = 0,
    WARN,
    ERR,
    CRIT,
    INFO,
    TRACE
};

auto get_process_args(const pid_t pid) -> std::string;
auto execute_command(const std::string_view command) -> std::string;

template <typename... Args>
auto log_message(LogLevel level, const std::string &fmt, Args&&... args) -> void {
    switch (level) {
        case NONE: break;
        case LOG: fmt::print("[{}] ", fmt::styled("LOG", fmt::fg(fmt::color::gray))); break;
        case WARN: fmt::print("[{}] ", fmt::styled("WARN", fmt::fg(fmt::color::yellow))); break;
        case ERR: fmt::print("[{}] ", fmt::styled("ERR", fmt::fg(fmt::color::orange))); break;
        case CRIT: fmt::print("[{}] ", fmt::styled("CRIT", fmt::fg(fmt::color::red))); break;
        case INFO: fmt::print("[{}] ", fmt::styled("INFO", fmt::fg(fmt::color::light_blue))); break;
        case TRACE: fmt::print("[{}] ", fmt::styled("TRACE", fmt::fg(fmt::color::light_gray))); break;
    }
    fmt::print(fmt::runtime(fmt), std::forward<Args>(args)...);
}
