#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>
#include <iostream>
#include <poost/log.hpp>

auto main() -> int {
    poost::log::global = poost::LogSettings{
        .stream = &std::cerr,
        .log_level = poost::LogLevel::Fatal,
        .use_colors = true,
    };

    doctest::Context context{};
    return context.run();
}
