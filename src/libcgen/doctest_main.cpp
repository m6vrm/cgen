#include <poost/log.hpp>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <cstdio>

auto main() -> int {
    poost::log::main = poost::LogSettings{
        .stream = stderr,
        .log_level = poost::LogLevel::Fatal,
        .print_file_line = true,
        .use_color = true,
    };

    doctest::Context context{};
    return context.run();
}
