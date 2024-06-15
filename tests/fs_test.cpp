#include <doctest/doctest.h>
#include <fs.hpp>

TEST_CASE("paths checking") {
    SUBCASE("relative subpath is valid") {
        CHECK(cgen::path_is_sub("subdir", "/path/to/dir"));
        CHECK(cgen::path_is_sub("../dir/subdir", "/path/to/dir"));
        CHECK(cgen::path_is_sub("./subdir", "/path/to/dir"));
    }

    SUBCASE("absolute subpath is valid") {
        CHECK(cgen::path_is_sub("/path/to/dir/subdir", "/path/to/dir"));
        CHECK(cgen::path_is_sub("/path/to/dir/../dir/subdir", "/path/to/dir"));
    }

    SUBCASE("relative path from other hierarchy is invalid") { //
        CHECK(cgen::path_is_sub("../subdir", "/path/to/dir") == false);
    }

    SUBCASE("absolute path from other hierarchy is invalid") { //
        CHECK(cgen::path_is_sub("/path/to/subdir", "/path/to/dir") == false);
    }
}
