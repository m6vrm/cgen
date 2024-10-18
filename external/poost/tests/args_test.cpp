#include <doctest/doctest.h>
#include <args.hpp>
#include <string>

TEST_CASE("Args") {
    const char* argv[] = {"test", "-ab", "-c", "-d", "value", "-e", "42"};
    const int argc = sizeof(argv) / sizeof(argv[0]);

    poost::Args args{argc, argv};

    bool a = false;
    bool b = false;
    bool c = false;
    std::string d;
    int e = 0;

    char opt;
    while ((opt = args.option()) != poost::args::end) {
        switch (opt) {
            case 'a':
                a = true;
                break;
            case 'b':
                b = true;
                break;
            case 'c':
                c = true;
                break;
            case 'd':
                args.value(d);
                break;
            case 'e':
                args.value(e);
                break;
            default:
                FAIL("unknown option");
                break;
        }
    }

    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == "value");
    CHECK(e == 42);
}
