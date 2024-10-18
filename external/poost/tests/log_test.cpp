#include <doctest/doctest.h>
#include <log.hpp>
#include <sstream>

TEST_CASE("Log") {
    std::ostringstream oss;

    SUBCASE("log level") {
        poost::LogSettings log_settings{
            .stream = &oss,
            .log_level = poost::LogLevel::ALL,
            .use_colors = false,
            .print_location = true,
        };

        POOST_TRACE_EX(log_settings, "Hello");
        POOST_INFO_EX(log_settings, "Hello");
        POOST_DEBUG_EX(log_settings, "Hello");
        POOST_WARN_EX(log_settings, "Hello");
        POOST_ERROR_EX(log_settings, "Hello");
        poost::log_print(log_settings, poost::LogLevel::FATAL, __FILE_NAME__, __LINE__,
                         __FUNCTION__, "Hello");

        CHECK(oss.str() ==
              "[TRACE] log_test.cpp(16) DOCTEST_ANON_FUNC_2: Hello\n"
              "[INFO ] log_test.cpp(17) DOCTEST_ANON_FUNC_2: Hello\n"
              "[DEBUG] log_test.cpp(18) DOCTEST_ANON_FUNC_2: Hello\n"
              "[WARN ] log_test.cpp(19) DOCTEST_ANON_FUNC_2: Hello\n"
              "[ERROR] log_test.cpp(20) DOCTEST_ANON_FUNC_2: Hello\n"
              "[FATAL] log_test.cpp(21) DOCTEST_ANON_FUNC_2: Hello\n");
    }

    SUBCASE("log level colors") {
        poost::LogSettings log_settings{
            .stream = &oss,
            .log_level = poost::LogLevel::ALL,
            .use_colors = true,
            .print_location = true,
        };

        POOST_TRACE_EX(log_settings, "Hello");
        POOST_INFO_EX(log_settings, "Hello");
        POOST_DEBUG_EX(log_settings, "Hello");
        POOST_WARN_EX(log_settings, "Hello");
        POOST_ERROR_EX(log_settings, "Hello");
        poost::log_print(log_settings, poost::LogLevel::FATAL, __FILE_NAME__, __LINE__,
                         __FUNCTION__, "Hello");

        CHECK(oss.str() ==
              "\x1b[37m[TRACE] log_test.cpp(41) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n"
              "\x1b[32m[INFO ] log_test.cpp(42) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n"
              "\x1b[34m[DEBUG] log_test.cpp(43) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n"
              "\x1b[33m[WARN ] log_test.cpp(44) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n"
              "\x1b[31m[ERROR] log_test.cpp(45) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n"
              "\x1b[35m[FATAL] log_test.cpp(46) DOCTEST_ANON_FUNC_2:\x1b[0m Hello\n");
    }

    SUBCASE("log without location") {
        poost::LogSettings log_settings{
            .stream = &oss,
            .log_level = poost::LogLevel::ALL,
            .use_colors = true,
            .print_location = false,
        };

        POOST_TRACE_EX(log_settings, "Hello");
        POOST_INFO_EX(log_settings, "Hello");
        POOST_DEBUG_EX(log_settings, "Hello");
        POOST_WARN_EX(log_settings, "Hello");
        POOST_ERROR_EX(log_settings, "Hello");
        poost::log_print(log_settings, poost::LogLevel::FATAL, __FILE_NAME__, __LINE__,
                         __FUNCTION__, "Hello");

        CHECK(oss.str() ==
              "\x1b[37m[TRACE]\x1b[0m Hello\n"
              "\x1b[32m[INFO ]\x1b[0m Hello\n"
              "\x1b[34m[DEBUG]\x1b[0m Hello\n"
              "\x1b[33m[WARN ]\x1b[0m Hello\n"
              "\x1b[31m[ERROR]\x1b[0m Hello\n"
              "\x1b[35m[FATAL]\x1b[0m Hello\n");
    }
}
