#include <doctest/doctest.h>
#include <packages.hpp>
#include <sstream>

using FetchStrategy = cgen::packages::FetchStrategy;

TEST_CASE("resolved packages reading and writing") {
    SUBCASE("read resolved packages with current version") {
        std::stringstream ss{R"(
            version 1
            s path1 url1 ver1 over1
            c path2 url2 ver2 over2
        )"};

        const std::vector<cgen::Package> resolved = cgen::resolved_read(ss);
        CHECK(resolved.size() == 2);
        CHECK(resolved[0].strategy == FetchStrategy::Submodule);
        CHECK(resolved[0].path == "path1");
        CHECK(resolved[0].url == "url1");
        CHECK(resolved[0].version == "ver1");
        CHECK(resolved[1].strategy == FetchStrategy::Clone);
        CHECK(resolved[1].path == "path2");
        CHECK(resolved[1].url == "url2");
        CHECK(resolved[1].version == "ver2");
    }

    SUBCASE("read packages are equal to written packages") {
        const std::vector<cgen::Package> write_resolved{
            cgen::Package{
                .strategy = FetchStrategy::Submodule,
                .path = "path1",
                .url = "url1",
                .version = "ver1",
                .original_version = "over1",
            },
            cgen::Package{
                .strategy = FetchStrategy::Clone,
                .path = "path2",
                .url = "url2",
                .version = "ver2",
                .original_version = "over2",
            },
        };

        std::stringstream ss;
        cgen::resolved_write(ss, write_resolved);
        const std::vector<cgen::Package> read_resolved = cgen::resolved_read(ss);
        CHECK(read_resolved.size() == 2);
        CHECK(read_resolved[0].strategy == FetchStrategy::Submodule);
        CHECK(read_resolved[0].path == "path1");
        CHECK(read_resolved[0].url == "url1");
        CHECK(read_resolved[0].version == "ver1");
        CHECK(read_resolved[0].original_version == "over1");
        CHECK(read_resolved[1].strategy == FetchStrategy::Clone);
        CHECK(read_resolved[1].path == "path2");
        CHECK(read_resolved[1].url == "url2");
        CHECK(read_resolved[1].original_version == "over2");
    }

    SUBCASE("empty packages if version is wrong") {
        std::stringstream ss{R"(
            version 0
            s path1 url1 ver1 over1
        )"};

        const std::vector<cgen::Package> resolved = cgen::resolved_read(ss);
        CHECK(resolved.empty());
    }

    SUBCASE("empty packages if input has wrong format") {
        std::stringstream ss{"hello world 42"};
        const std::vector<cgen::Package> resolved = cgen::resolved_read(ss);
        CHECK(resolved.empty());
    }

    SUBCASE("empty packages if input is empty") {
        std::stringstream ss;
        const std::vector<cgen::Package> resolved = cgen::resolved_read(ss);
        CHECK(resolved.empty());
    }
}
