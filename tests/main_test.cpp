#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>
#include <iostream>
#include <poost/log.hpp>

auto main() -> int {
    poost::log::global = poost::LogSettings{
        .stream = &std::cerr,
        .log_level = poost::LogLevel::FATAL,
        .prefix = nullptr,
        .use_colors = true,
        .print_location = true,
    };

    doctest::Context context{};
    return context.run();
}
