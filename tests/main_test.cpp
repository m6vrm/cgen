#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>
#include <poost/log.hpp>
#include <cstdio>

auto main() -> int {
    poost::log::global = poost::LogSettings{
        .stream = stderr,
        .log_level = poost::LogLevel::Fatal,
        .use_colors = true,
    };

    doctest::Context context{};
    return context.run();
}
