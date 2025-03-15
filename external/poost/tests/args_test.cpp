#include <doctest/doctest.h>
#include <args.hpp>
#include <string>

TEST_CASE("Args") {
    const char* argv[] = {"test", "-ab", "-c", "-d", "value", "-e", "42"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    bool a = false;
    bool b = false;
    bool c = false;
    std::string d;
    int e = 0;

    poost::Args args{argc, argv};
    for (char option = args.option(); option != poost::args::end; option = args.option()) {
        switch (option) {
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
                FAIL("unknown option: ", option);
                break;
        }
    }

    CHECK(a == true);
    CHECK(b == true);
    CHECK(c == true);
    CHECK(d == "value");
    CHECK(e == 42);
}
