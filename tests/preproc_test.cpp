#include <debug.hpp>
#include <doctest/doctest.h>
#include <preproc.hpp>

/// Merge

TEST_CASE("node merging") {
    YAML::Node from = YAML::Load(R"(
        list: [1, 2, 3]
        map: { hello: world, nested: { key: value } }
        scalar: something
    )");

    SUBCASE("source replaces empty destination") {
        YAML::Node to = YAML::Load("");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{list: [1, 2, 3], map: {hello: world, nested: {key: value}}, "
              "scalar: something}");
    }

    SUBCASE("source replaces uninitialized destination") {
        YAML::Node to;
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{list: [1, 2, 3], map: {hello: world, nested: {key: value}}, "
              "scalar: something}");
    }

    SUBCASE("source appends to the list") {
        YAML::Node to = YAML::Load("list: [4, 5]");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{list: [4, 5, 1, 2, 3], map: {hello: world, nested: {key: "
              "value}}, scalar: something}");
    }

    SUBCASE("source merges into the map") {
        YAML::Node to =
            YAML::Load("map: { hello: hello, nested: { key: nothing } }");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{map: {hello: world, nested: {key: value}}, list: [1, 2, 3], "
              "scalar: something}");
    }

    SUBCASE("source replaces scalar") {
        YAML::Node to = YAML::Load("scalar: nothing");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{scalar: something, list: [1, 2, 3], map: {hello: world, "
              "nested: {key: value}}}");
    }

    SUBCASE("destination keeps unaffected nodes") {
        YAML::Node to = YAML::Load("key: value");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{key: value, list: [1, 2, 3], map: {hello: world, nested: {key: "
              "value}}, scalar: something}");
    }

    SUBCASE("keep destination on empty source") {
        YAML::Node to = YAML::Load("");
        cgen::node_merge(to, from);
        CHECK(cgen::node_dump(from) ==
              "{list: [1, 2, 3], map: {hello: world, nested: {key: value}}, "
              "scalar: something}");
    }

    SUBCASE("keep destination on uninitialized source") {
        YAML::Node to;
        cgen::node_merge(to, from);
        CHECK(cgen::node_dump(from) ==
              "{list: [1, 2, 3], map: {hello: world, nested: {key: value}}, "
              "scalar: something}");
    }
}

/// Replace

TEST_CASE("node replacing") {
    SUBCASE("source replaces empty destination without attributes") {
        const YAML::Node from = YAML::Load("scalar:REPLACE: something");
        YAML::Node to = YAML::Load("");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{scalar: something}");
    }

    SUBCASE("source replaces uninitialized destination without attributes") {
        const YAML::Node from = YAML::Load("scalar:REPLACE: something");
        YAML::Node to;
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{scalar: something}");
    }

    SUBCASE("keep destination scalar") {
        const YAML::Node from = YAML::Load("scalar: something");
        YAML::Node to = YAML::Load("scalar:REPLACE: nothing");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{scalar:REPLACE: nothing}");
    }

    SUBCASE("keep destination list") {
        const YAML::Node from = YAML::Load("list: [1, 2, 3]");
        YAML::Node to = YAML::Load("list:REPLACE: [4]");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{list:REPLACE: [4]}");
    }

    SUBCASE("keep destination map") {
        const YAML::Node from =
            YAML::Load("map: { something: hello, key: value }");
        YAML::Node to =
            YAML::Load("map:REPLACE: { hello: world, key: nothing }");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{map:REPLACE: {hello: world, key: nothing}}");
    }

    SUBCASE("keep destination scalar") {
        const YAML::Node from = YAML::Load("scalar:REPLACE: something");
        YAML::Node to = YAML::Load("scalar:REPLACE: nothing");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{scalar:REPLACE: nothing}");
    }

    SUBCASE("keep destination list") {
        const YAML::Node from = YAML::Load("list:REPLACE: [1, 2, 3]");
        YAML::Node to = YAML::Load("list:REPLACE: [4]");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{list:REPLACE: [4]}");
    }

    SUBCASE("keep destination map") {
        const YAML::Node from =
            YAML::Load("map:REPLACE: { something: hello, key: value }");
        YAML::Node to =
            YAML::Load("map:REPLACE: { hello: world, key: nothing }");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) ==
              "{map:REPLACE: {hello: world, key: nothing}}");
    }

    SUBCASE("replace destination scalar") {
        const YAML::Node from = YAML::Load("scalar:REPLACE: something");
        YAML::Node to = YAML::Load("scalar: nothing");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{scalar: something}");
    }

    SUBCASE("replace destination list") {
        const YAML::Node from = YAML::Load("list:REPLACE: [1, 2, 3]");
        YAML::Node to = YAML::Load("list: [4]");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{list: [1, 2, 3]}");
    }

    SUBCASE("replace destination map") {
        const YAML::Node from =
            YAML::Load("map:REPLACE: { something: hello, key: value }");
        YAML::Node to = YAML::Load("map: { hello: world, key: nothing }");
        cgen::node_merge(from, to);
        CHECK(cgen::node_dump(to) == "{map: {something: hello, key: value}}");
    }
}

/// Parameters

TEST_CASE("parameters replacing") {
    SUBCASE("parameters replacing in scalar") {
        YAML::Node node = YAML::Load("some $(key)");
        std::vector<std::string> undefined_params;
        cgen::node_replace_parameters(node, {{"key", "value"}},
                                      undefined_params);
        CHECK(undefined_params.empty());
        CHECK(cgen::node_dump(node) == "some value");
    }

    SUBCASE("parameters replacing in list") {
        YAML::Node node = YAML::Load("[ some $(key), $(another) ]");
        std::vector<std::string> undefined_params;
        cgen::node_replace_parameters(
            node, {{"key", "value"}, {"another", "another value"}},
            undefined_params);
        CHECK(undefined_params.empty());
        CHECK(cgen::node_dump(node) == "[some value, another value]");
    }

    SUBCASE("parameters replacing in map") {
        YAML::Node node = YAML::Load("{ some: $(key), another: $(another) }");
        std::vector<std::string> undefined_params;
        cgen::node_replace_parameters(
            node, {{"key", "value"}, {"another", "another value"}},
            undefined_params);
        CHECK(undefined_params.empty());
        CHECK(cgen::node_dump(node) == "{some: value, another: another value}");
    }

    SUBCASE("parameters escaping") {
        YAML::Node node = YAML::Load("$ $! $(key) $$(key) $$ $");
        std::vector<std::string> undefined_params;
        cgen::node_replace_parameters(node, {{"key", "value"}},
                                      undefined_params);
        CHECK(undefined_params.empty());
        CHECK(cgen::node_dump(node) == "$ $! value $(key) $ $");
    }

    SUBCASE("error on undefined parameters") {
        YAML::Node node = YAML::Load("some $(undefined1)$(key)$(undefined2)");
        std::vector<std::string> undefined_params;
        cgen::node_replace_parameters(node, {{"key", "value"}},
                                      undefined_params);
        CHECK(undefined_params.size() == 2);
        CHECK(undefined_params[0] == "undefined1");
        CHECK(undefined_params[1] == "undefined2");
        CHECK(cgen::node_dump(node) == "some value");
    }
}

/// Attributes

TEST_CASE("attributes trimming") {
    SUBCASE("trim map attributes") {
        YAML::Node node = YAML::Load(R"(
        map:ATTR:
          key1:ATTR: value1
          key2: value2
        )");

        cgen::node_trim_attributes(node);
        CHECK(cgen::node_dump(node) == "{map: {key1: value1, key2: value2}}");
    }

    SUBCASE("keep list attributes") {
        YAML::Node node = YAML::Load(R"(
        list:
          - key1:ATTR: value1
          - key2: value2
        )");

        cgen::node_trim_attributes(node);
        CHECK(cgen::node_dump(node) ==
              "{list: [{key1:ATTR: value1}, {key2: value2}]}");
    }
}

/// Wrapping

TEST_CASE("configs wrapping") {
    SUBCASE("wrap configs") {
        YAML::Node node = YAML::Load("public: [ 1, 2, 3 ]");
        cgen::node_wrap_configs(node, "public");
        CHECK(cgen::node_dump(node) == "{public: {global: [1, 2, 3]}}");
    }

    SUBCASE("wrap configs with REPLACE attr") {
        YAML::Node node = YAML::Load("public:REPLACE: [ 1, 2, 3 ]");
        cgen::node_wrap_configs(node, "public");
        CHECK(cgen::node_dump(node) == "{public:REPLACE: {global: [1, 2, 3]}}");
    }

    SUBCASE("don't wrap configs with correct nested fields") {
        YAML::Node node = YAML::Load(R"(
        public:
          configurations:
            Release: [ 1, 2, 3 ]
        )");

        cgen::node_wrap_configs(node, "public");
        CHECK(cgen::node_dump(node) ==
              "{public: {configurations: {Release: [1, 2, 3]}}}");
    }
}

TEST_CASE("visibility wrapping") {
    SUBCASE("wrap visibility") {
        YAML::Node node = YAML::Load("key: [ 1, 2, 3 ]");
        cgen::node_wrap_visibility(node, "key");
        CHECK(cgen::node_dump(node) == "{key: {default: {global: [1, 2, 3]}}}");
    }

    SUBCASE("wrap visibility with REPLACE attr") {
        YAML::Node node = YAML::Load("key:REPLACE: [ 1, 2, 3 ]");
        cgen::node_wrap_visibility(node, "key");
        CHECK(cgen::node_dump(node) ==
              "{key:REPLACE: {default: {global: [1, 2, 3]}}}");
    }

    SUBCASE("wrap visibility with specifier") {
        YAML::Node node = YAML::Load(R"(
        key:
          public: [ 1, 2, 3 ]
        )");

        cgen::node_wrap_visibility(node, "key");
        CHECK(cgen::node_dump(node) == "{key: {public: {global: [1, 2, 3]}}}");
    }

    SUBCASE("wrap visibility with specifier and configs") {
        YAML::Node node = YAML::Load(R"(
        key:
          public:
            configurations:
              Release: [ 1, 2, 3 ]
          private: [ 4, 5, 6 ]
        )");

        cgen::node_wrap_visibility(node, "key");
        CHECK(cgen::node_dump(node) ==
              "{key: {public: {configurations: {Release: [1, 2, 3]}}, "
              "private: {global: [4, 5, 6]}}}");
    }
}
