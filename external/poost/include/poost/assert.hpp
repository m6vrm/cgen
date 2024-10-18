#pragma once

#include <utility>

#ifndef NDEBUG
#include <cstdlib>
#include <poost/log.hpp>
#define POOST_ASSERT(condition, ...)                                                              \
    do {                                                                                          \
        if (!(condition)) {                                                                       \
            poost::log_print(poost::log::global, poost::LogLevel::FATAL, __FILE_NAME__, __LINE__, \
                             __FUNCTION__, "assertion failure ({})", #condition);                 \
            poost::log_print(poost::log::global, poost::LogLevel::FATAL, __FILE_NAME__, __LINE__, \
                             __FUNCTION__, __VA_ARGS__);                                          \
            std::abort();                                                                         \
        }                                                                                         \
    } while (false)
#else
#define POOST_ASSERT(cond, ...)
#endif

#define POOST_ASSERT_FAIL(...)            \
    do {                                  \
        POOST_ASSERT(false, __VA_ARGS__); \
        std::unreachable();               \
    } while (false)
