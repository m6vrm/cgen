#include <algorithm>
#include <doctest/doctest.h>
#include <versions.hpp>

auto version_tag(const std::string &ver, const std::vector<std::string> &tags,
                 bool ignore_rc = false) -> std::optional<std::string> {

    const std::optional<std::string> tag =
        cgen::version_tag(ver, tags, ignore_rc);

    std::vector<std::string> reversed_tags{tags};
    std::reverse(reversed_tags.begin(), reversed_tags.end());
    const std::optional<std::string> reversed_tag =
        cgen::version_tag(ver, tags, ignore_rc);

    CHECK(tag == reversed_tag);

    return tag;
}

auto version_less(const std::string &lhs, const std::string &rhs) -> bool {
    const bool less = cgen::version_less(lhs, rhs);
    const bool greater = cgen::version_less(rhs, lhs);
    return less == !greater;
}

TEST_CASE("checking version is valid") {
    SUBCASE("tag name is not a valid version") {
        const std::string ver = "v1.0";
        CHECK(cgen::version_is_valid(ver) == false);
    }

    SUBCASE("branch name is not a valid version") {
        const std::string ver = "branch-name-1.0";
        CHECK(cgen::version_is_valid(ver) == false);
    }

    SUBCASE("commit hash is not a valid version") {
        const std::string ver = "deadbeef";
        CHECK(cgen::version_is_valid(ver) == false);
    }

    SUBCASE("simple version is valid") {
        const std::string ver = "1.0";
        CHECK(cgen::version_is_valid(ver));
    }

    SUBCASE("version with wildcard is valid") {
        const std::string ver = "1.*";
        CHECK(cgen::version_is_valid(ver));
    }

    SUBCASE("just wildcard is a valid version") {
        const std::string ver = "*";
        CHECK(cgen::version_is_valid(ver));
    }
}

TEST_CASE("version matching") {
    SUBCASE("simple tag matching") {
        const std::string tag = "1.2.3";
        CHECK(cgen::version_match("1.2.3", tag));
        CHECK(cgen::version_match("1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3", tag));
        CHECK(cgen::version_match("v1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3.0.*", tag));
        CHECK(cgen::version_match("1.*", tag));
        CHECK(cgen::version_match("1.2.*", tag));
        CHECK(cgen::version_match("1.2.3.*", tag));
        CHECK(cgen::version_match("1.*.3.*", tag));
        CHECK(cgen::version_match("1.*.*.0", tag));
        CHECK(cgen::version_match("*.2.3.*", tag));
        CHECK(cgen::version_match("*", tag));
    }

    SUBCASE("prefixed tag matching") {
        const std::string tag = "v1.2.3";
        CHECK(cgen::version_match("1.2.3", tag));
        CHECK(cgen::version_match("1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3", tag));
        CHECK(cgen::version_match("v1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3.0.*", tag));
        CHECK(cgen::version_match("1.*", tag));
        CHECK(cgen::version_match("1.2.*", tag));
        CHECK(cgen::version_match("1.2.3.*", tag));
        CHECK(cgen::version_match("1.*.3.*", tag));
        CHECK(cgen::version_match("1.*.*.0", tag));
        CHECK(cgen::version_match("*.2.3.*", tag));
        CHECK(cgen::version_match("*", tag));
    }

    SUBCASE("trailing zeros tag matching") {
        const std::string tag = "v1.2.3.0";
        CHECK(cgen::version_match("1.2.3", tag));
        CHECK(cgen::version_match("1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3", tag));
        CHECK(cgen::version_match("v1.2.3.0", tag));
        CHECK(cgen::version_match("v1.2.3.0.*", tag));
        CHECK(cgen::version_match("1.*", tag));
        CHECK(cgen::version_match("1.2.*", tag));
        CHECK(cgen::version_match("1.2.3.*", tag));
        CHECK(cgen::version_match("1.*.3.*", tag));
        CHECK(cgen::version_match("1.*.*.0", tag));
        CHECK(cgen::version_match("*.2.3.*", tag));
        CHECK(cgen::version_match("*", tag));
    }

    SUBCASE("failing tag matching") {
        const std::string tag = "v1.2.3.0";
        CHECK(cgen::version_match("1.2", tag) == false);
        CHECK(cgen::version_match("1.3", tag) == false);
        CHECK(cgen::version_match("1.2.3.1", tag) == false);
        CHECK(cgen::version_match("v1.2.3.0.1", tag) == false);
        CHECK(cgen::version_match("1.1.*", tag) == false);
        CHECK(cgen::version_match("*.1", tag) == false);
    }

    SUBCASE("ignore or respect pre-releases") {
        CHECK(cgen::version_match("1.2.3", "v1.2.3-rc1", true) == false);
        CHECK(cgen::version_match("1.2.3", "v1.2.3-rc1", false) == true);
    }
}

TEST_CASE("version comparison") {
    SUBCASE("prefixed version is preferred") {
        CHECK(version_less("1.0.0", "v1.0.0"));
    }

    SUBCASE("longest version is preferred") {
        CHECK(version_less("1.0", "1.0.0"));
    }

    SUBCASE("semver comparison") {
        // see https://semver.org/spec/v2.0.0-rc.1.html
        CHECK(version_less("1.0.0-alpha", "1.0.0-alpha.1"));
        CHECK(version_less("1.0.0-alpha.1", "1.0.0-beta.2"));
        CHECK(version_less("1.0.0-beta.2", "1.0.0-beta.11"));
        // CHECK(version_less("1.0.0-beta.11", "1.0.0-rc.1"));
        CHECK(version_less("1.0.0-rc.1", "1.0.0-rc.1+build.1"));
        CHECK(version_less("1.0.0-rc.1+build.1", "1.0.0"));
        CHECK(version_less("1.0.0", "1.0.0+0.3.7"));
        CHECK(version_less("1.0.0+0.3.7", "1.3.7+build"));
        CHECK(version_less("1.3.7+build", "1.3.7+build.2.b8f12d7"));
        CHECK(version_less("1.3.7+build.2.b8f12d7", "1.3.7+build.11.e0f985a"));

        CHECK(version_less("v1.2.3-rc1", "v1.2.3"));
    }

    SUBCASE("lexicographical comparison") {
        CHECK(version_less("1.0", "1.0.1"));
        CHECK(version_less("1", "2"));
        CHECK(version_less("1.0", "2"));
        CHECK(version_less("1.99", "2"));
        CHECK(version_less("1.2", "1.11"));
    }

    SUBCASE("wildcard comparison") {
        CHECK(version_less("1.0", "1.*"));
        CHECK(version_less("1.0.1", "1.*"));
        CHECK(version_less("1.0.*", "1.1.0"));
        CHECK(version_less("999", "*"));
    }
}

TEST_CASE("tag searching") {
    const std::vector<std::string> tags{
        "0.1",       //
        "v1.0",      //
        "1.0.0",     //
        "1.2.3-rc1", //
        "1.2.3",     //
        "v1.2.3",    //
        "1.2.4-rc1", //
        "1.2.4-rc2", //
        "2",         //
        "v2.0.1",    //
        "2.3",       //
    };

    SUBCASE("exact tag found") {
        const std::optional<std::string> tag = version_tag("0.1", tags);
        CHECK(tag.value() == "0.1");
    }

    SUBCASE("prefixed tag found") {
        const std::optional<std::string> tag = version_tag("1.2.3", tags);
        CHECK(tag.value() == "v1.2.3");
    }

    SUBCASE("longest tag found") {
        const std::optional<std::string> tag = version_tag("1.0", tags);
        CHECK(tag.value() == "1.0.0");
    }

    SUBCASE("tag found by pattern 2.*") {
        const std::optional<std::string> tag = version_tag("2.*", tags);
        CHECK(tag.value() == "2.3");
    }

    SUBCASE("tag found by pattern 2.0.0.*") {
        const std::optional<std::string> tag = version_tag("2.0.0.*", tags);
        CHECK(tag.value() == "2");
    }

    SUBCASE("tag found by pattern 2.*.1") {
        const std::optional<std::string> tag = version_tag("2.*.1", tags);
        CHECK(tag.value() == "v2.0.1");
    }

    SUBCASE("exact tag not found") {
        const std::optional<std::string> tag = version_tag("0.2", tags);
        CHECK(tag.has_value() == false);
    }

    SUBCASE("tag not found by pattern 1.*.1") {
        const std::optional<std::string> tag = version_tag("1.*.1", tags);
        CHECK(tag.has_value() == false);
    }

    SUBCASE("ignore or respect pre-releases") {
        CHECK(version_tag("1.*.4", tags, true).has_value() == false);
        CHECK(version_tag("1.*.4", tags, false).value() == "1.2.4-rc2");
    }

    SUBCASE("max version found by just wildcard") {
        const std::optional<std::string> tag = version_tag("*", tags);
        CHECK(tag.value() == "2.3");
    }
}
