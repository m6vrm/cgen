#pragma once

#include <cstdlib>
#include <ostream>
#include <string_view>
#include "format.hpp"

#define POOST_TRACE(...) POOST_TRACE_EX(poost::log::global, __VA_ARGS__)
#define POOST_DEBUG(...) POOST_DEBUG_EX(poost::log::global, __VA_ARGS__)
#define POOST_INFO(...) POOST_INFO_EX(poost::log::global, __VA_ARGS__)
#define POOST_WARN(...) POOST_WARN_EX(poost::log::global, __VA_ARGS__)
#define POOST_ERROR(...) POOST_ERROR_EX(poost::log::global, __VA_ARGS__)
#define POOST_FATAL(...) POOST_FATAL_EX(poost::log::global, __VA_ARGS__)

#define POOST_LOG(...) POOST_LOG_EX(poost::log::global, __VA_ARGS__)

#define POOST_TRACE_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::TRACE, __VA_ARGS__)
#define POOST_DEBUG_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::DEBUG, __VA_ARGS__)
#define POOST_INFO_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::INFO, __VA_ARGS__)
#define POOST_WARN_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::WARN, __VA_ARGS__)
#define POOST_ERROR_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::ERROR, __VA_ARGS__)
#define POOST_FATAL_EX(settings, ...) POOST_LOG_EX(settings, poost::LogLevel::FATAL, __VA_ARGS__)

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define POOST_LOG_EX(settings, level, ...)                                               \
    do {                                                                                 \
        if ((level) >= (settings).log_level) {                                           \
            poost::log_print((settings), (level), __FILE_NAME__, __LINE__, __FUNCTION__, \
                             __VA_ARGS__);                                               \
                                                                                         \
            if ((level) == poost::LogLevel::FATAL) {                                     \
                std::abort();                                                            \
            }                                                                            \
        }                                                                                \
    } while (false)

namespace poost {

enum class LogLevel {
    ALL,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NONE,
};

struct LogSettings {
    std::ostream* stream;
    LogLevel log_level;
    bool use_colors;
    bool print_location;
};

namespace log {

extern LogSettings global;

}  // namespace log

std::string_view log_level_label(LogLevel level);
std::string_view log_level_color(LogLevel level);

template <typename... Args>
void log_print(const LogSettings& settings,
               LogLevel level,
               std::string_view file,
               int line,
               std::string_view function,
               fmt::format_string<Args...> format,
               Args&&... args) {
    if (settings.use_colors) {
        *settings.stream << log_level_color(level);
    }

    *settings.stream << fmt::format("[{:<5}]", log_level_label(level));

    if (settings.print_location) {
        *settings.stream << fmt::format(" {}({}) {}:", file, line, function);
    }

    if (settings.use_colors) {
        *settings.stream << "\x1b[0m";
    }

    *settings.stream << " " << fmt::format(format, std::forward<Args>(args)...) << "\n";
}

}  // namespace poost
