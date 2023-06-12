#include "config.hpp"
#include "debug.hpp"
#include "mocks.hpp"

#include <doctest/doctest.h>

#include <sstream>

auto config_read(std::istream &in, std::vector<cgen::Error> &errors) {
    return cgen::config_read(in, 1, errors);
}

/// Parsing

TEST_CASE("invalid config parsing") {
    SUBCASE("empty config parsed with errors") {
        std::istringstream config_iss{""};
        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "config validation error: /project: node not found");
    }
}

TEST_CASE("version parsing") {
    SUBCASE("scalar value parsed correctly") {
        std::istringstream config_iss{R"(
        version: 1
        project: project name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.version == "1");
    }

    SUBCASE("no error if config has no version") {
        std::istringstream config_iss{R"(
        project: project name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
    }

    SUBCASE("error if config version is unsupported") {
        std::istringstream config_iss{R"(
        version: 2
        project: project name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "unsupported config version: 2");
    }
}

TEST_CASE("project parsing") {
    SUBCASE("scalar value parsed as project name correctly") {
        std::istringstream config_iss{R"(
        project: project name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.project.name == "project name");
        CHECK(config.project.version == "");
    }

    SUBCASE("map value parsed as project name and version correctly") {
        std::istringstream config_iss{R"(
        project:
          name: project name
          version: 1
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.project.name == "project name");
        CHECK(config.project.version == "1");
    }

    SUBCASE("validation error if project is missing") {
        std::istringstream config_iss{""};
        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "config validation error: /project: node not found");
    }

    SUBCASE("validation error if project name is missing") {
        std::istringstream config_iss{R"(
        project:
          version: 1
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /project: expected value type: project"
              "\n\t* failed variant 0:"
              "\n\t\t/project: expected value type: string"
              "\n\t* failed variant 1:"
              "\n\t\t/project.name: node not found");
    }
}

TEST_CASE("includes parsing") {
    SUBCASE("list of paths parsed correctly") {
        cgen::mock_files({
            {"empty1", ""},
            {"empty2", ""},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - empty1
          - empty2
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.includes.size() == 2);
        CHECK(config.includes[0].paths.size() == 1);
        CHECK(config.includes[0].paths[0] == "empty1");
        CHECK(config.includes[0].parameters.empty());
        CHECK(config.includes[1].paths.size() == 1);
        CHECK(config.includes[1].paths[0] == "empty2");
        CHECK(config.includes[1].parameters.empty());
    }

    SUBCASE("list of paths with parameters parsed correctly") {
        cgen::mock_files({
            {"empty1", ""},
            {"empty2", ""},
            {"empty3", ""},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - paths: [ empty1, empty2 ]
          - paths: [ empty3 ]
            parameters:
              param1: value1
              param2: value2
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.includes.size() == 2);
        CHECK(config.includes[0].paths.size() == 2);
        CHECK(config.includes[0].paths[0] == "empty1");
        CHECK(config.includes[0].paths[1] == "empty2");
        CHECK(config.includes[0].parameters.empty());
        CHECK(config.includes[1].paths.size() == 1);
        CHECK(config.includes[1].paths[0] == "empty3");
        CHECK(config.includes[1].parameters.size() == 2);
        CHECK(config.includes[1].parameters.at("param1") == "value1");
        CHECK(config.includes[1].parameters.at("param2") == "value2");
    }

    SUBCASE("validation error if include paths are missing") {
        std::istringstream config_iss{R"(
        project: project name
        includes:
          - parameters:
              param1: value1
              param2: value2
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /includes: expected value type: includes"
              "\n\t* failed variant 0:"
              "\n\t\t/includes.0: expected value type: string"
              "\n\t* failed variant 1:"
              "\n\t\t/includes.0.paths: node not found");
    }
}

TEST_CASE("templates parsing") {
    SUBCASE("simple template parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: []
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.templates.size() == 1);
        CHECK(cgen::node_dump(config.templates.at("template1").node) ==
              "{sources: {default: {global: []}}}");
    }

    SUBCASE("validation error if template is invalid") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template: invalid
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /templates.template: expected value type: target_settings");
    }
}

TEST_CASE("options parsing") {
    SUBCASE("option with default value parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        options:
          OPTION:
            description: option description
            default: default value
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.options.size() == 1);
        CHECK(config.options.at("OPTION").description == "option description");
        CHECK(config.options.at("OPTION").default_.is_defined);
        CHECK(config.options.at("OPTION").default_.is_quoted == false);
        CHECK(config.options.at("OPTION").default_.value == "default value");
    }

    SUBCASE("option without default value parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        options:
          OPTION:
            description: option description
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.options.size() == 1);
        CHECK(config.options.at("OPTION").default_.is_defined == false);
        CHECK(config.options.at("OPTION").default_.is_quoted == false);
        CHECK(config.options.at("OPTION").description == "option description");
    }

    SUBCASE("option with quoted default value parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        options:
          OPTION:
            description: option description
            default: "default value"
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.options.size() == 1);
        CHECK(config.options.at("OPTION").description == "option description");
        CHECK(config.options.at("OPTION").default_.is_quoted);
        CHECK(config.options.at("OPTION").default_.value == "default value");
    }

    SUBCASE("validation error if option has no description") {
        std::istringstream config_iss{R"(
        project: project name
        options:
          OPTION:
            default: default value
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /options.OPTION.description: node not found");
    }
}

TEST_CASE("settings parsing") {
    SUBCASE("settings parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        settings:
          VAR1: value
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.settings.size() == 1);
        CHECK(config.settings.at("VAR1").is_quoted == false);
        CHECK(config.settings.at("VAR1").value == "value");
    }

    SUBCASE("settings with quoted value parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        settings:
          VAR1: "value"
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.settings.size() == 1);
        CHECK(config.settings.at("VAR1").is_quoted);
        CHECK(config.settings.at("VAR1").value == "value");
    }
}

TEST_CASE("external packages parsing") {
    SUBCASE("external package parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        packages:
          - external: package name
            if: condition
            url: http://example.com
            version: 1
            strategy: clone
            options:
              OPTION1: value
              OPTION2: "quoted value"
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.packages.size() == 1);
        CHECK(config.packages[0].type == cgen::config::PackageType::External);
        CHECK(config.packages[0].name == "package name");
        CHECK(config.packages[0].if_ == "condition");
        CHECK(config.packages[0].external.url == "http://example.com");
        CHECK(config.packages[0].external.version == "1");
        CHECK(config.packages[0].external.strategy == cgen::config::FetchStrategy::Clone);
        CHECK(config.packages[0].external.options.size() == 2);
        CHECK(config.packages[0].external.options.at("OPTION1").is_quoted == false);
        CHECK(config.packages[0].external.options.at("OPTION1").value == "value");
        CHECK(config.packages[0].external.options.at("OPTION2").is_quoted);
        CHECK(config.packages[0].external.options.at("OPTION2").value == "quoted value");
    }

    SUBCASE("external package defaults") {
        std::istringstream config_iss{R"(
        project: project name
        packages:
          - external: package name
            url: http://example.com
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.packages.size() == 1);
        CHECK(config.packages[0].type == cgen::config::PackageType::External);
        CHECK(config.packages[0].name == "package name");
        CHECK(config.packages[0].if_ == "");
        CHECK(config.packages[0].external.url == "http://example.com");
        CHECK(config.packages[0].external.version == "");
        CHECK(config.packages[0].external.strategy == cgen::config::FetchStrategy::Submodule);
        CHECK(config.packages[0].external.options.empty());
    }

    SUBCASE("validation error if external package has missing url") {
        std::istringstream config_iss{R"(
        project: project name
        packages:
          - external: package name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /packages.0: expected value type: package"
              "\n\t* failed variant 0:"
              "\n\t\t/packages.0.url: node not found"
              "\n\t* failed variant 1:"
              "\n\t\t/packages.0.system: node not found"
              "\n\t\t/packages.0.external: undefined node");
    }
}

TEST_CASE("system packages parsing") {
    SUBCASE("system package parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        packages:
          - system: package name
            if: condition
            version: 2
            required: false
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.packages.size() == 1);
        CHECK(config.packages[0].type == cgen::config::PackageType::System);
        CHECK(config.packages[0].name == "package name");
        CHECK(config.packages[0].if_ == "condition");
        CHECK(config.packages[0].system.version == "2");
        CHECK(config.packages[0].system.is_required == false);
    }

    SUBCASE("system package defaults") {
        std::istringstream config_iss{R"(
        project: project name
        packages:
          - system: package name
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.packages.size() == 1);
        CHECK(config.packages[0].type == cgen::config::PackageType::System);
        CHECK(config.packages[0].name == "package name");
        CHECK(config.packages[0].if_ == "");
        CHECK(config.packages[0].system.version == "");
        CHECK(config.packages[0].system.is_required);
    }
}

TEST_CASE("library targets parsing") {
    SUBCASE("library target parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: static
            aliases: [ my::lib ]
            if: condition
            path: path/to/lib
            options:
              OPTION1:
                description: option description
                default: default value
            settings:
              VAR1: var value
            sources: [ path/to/source/file ]
            includes: [ path/to/include/dir ]
            pchs: [ path/to/pch ]
            dependencies: [ lib1, my::lib2 ]
            definitions:
              - DEFINE1: define value
              - DEFINE2
            properties:
              PROPERTY1: property value
            compile_options:
              - compile option
            link_options:
              - link option
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        // target
        CHECK(config.targets[0].type == cgen::config::TargetType::Library);
        CHECK(config.targets[0].name == "library name");
        // library type
        CHECK(config.targets[0].library.type == cgen::config::LibraryType::Static);
        // aliases
        CHECK(config.targets[0].library.aliases.size() == 1);
        CHECK(config.targets[0].library.aliases[0] == "my::lib");
        // if
        CHECK(config.targets[0].if_ == "condition");
        // path
        CHECK(config.targets[0].library.target_settings.path.value == "path/to/lib");
        // options
        CHECK(config.targets[0].library.target_settings.options.size() == 1);
        CHECK(config.targets[0].library.target_settings.options.at("OPTION1").description ==
              "option description");
        CHECK(config.targets[0].library.target_settings.options.at("OPTION1").default_.value ==
              "default value");
        // settings
        CHECK(config.targets[0].library.target_settings.settings.size() == 1);
        CHECK(config.targets[0].library.target_settings.settings.at("VAR1").value == "var value");
        // sources
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
        // includes
        CHECK(config.targets[0].library.target_settings.includes.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.includes.private_.global[0].value ==
              "path/to/include/dir");
        // pchs
        CHECK(config.targets[0].library.target_settings.pchs.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.pchs.private_.global[0].value ==
              "path/to/pch");
        // dependencies
        CHECK(config.targets[0].library.target_settings.dependencies.private_.global.size() == 2);
        CHECK(config.targets[0].library.target_settings.dependencies.private_.global[0].value ==
              "lib1");
        CHECK(config.targets[0].library.target_settings.dependencies.private_.global[1].value ==
              "my::lib2");
        // definitions
        CHECK(config.targets[0].library.target_settings.definitions.private_.global.size() == 2);
        CHECK(config.targets[0]
                  .library.target_settings.definitions.private_.global[0]
                  .value.value.empty());
        CHECK(config.targets[0].library.target_settings.definitions.private_.global[0].map.size() ==
              1);
        CHECK(config.targets[0]
                  .library.target_settings.definitions.private_.global[0]
                  .map.at("DEFINE1")
                  .value == "define value");
        CHECK(
            config.targets[0].library.target_settings.definitions.private_.global[1].value.value ==
            "DEFINE2");
        CHECK(config.targets[0].library.target_settings.definitions.private_.global[1].map.empty());
        // properties
        CHECK(config.targets[0].library.target_settings.properties.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.properties.global.at("PROPERTY1").value ==
              "property value");
        // compile_options
        CHECK(config.targets[0].library.target_settings.compile_options.private_.global.size() ==
              1);
        CHECK(config.targets[0].library.target_settings.compile_options.private_.global[0].value ==
              "compile option");
        // link_options
        CHECK(config.targets[0].library.target_settings.link_options.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.link_options.private_.global[0].value ==
              "link option");
    }

    SUBCASE("static library type parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: static
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.type == cgen::config::LibraryType::Static);
    }

    SUBCASE("shared library type parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: shared
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.type == cgen::config::LibraryType::Shared);
    }

    SUBCASE("interface library type parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: interface
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.type == cgen::config::LibraryType::Interface);
    }

    SUBCASE("validation error if target has invalid type") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - invalid
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() ==
              "config validation error: /targets.0: expected value type: target"
              "\n\t* failed variant 0:"
              "\n\t\t/targets.0.library: node not found"
              "\n\t* failed variant 1:"
              "\n\t\t/targets.0.executable: node not found");
    }
}

TEST_CASE("executable targets parsing") {
    SUBCASE("executable target parsed correctly") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - executable: executable name
            if: condition
            path: path/to/executable
            options:
              OPTION1:
                description: option description
                default: default value
            settings:
              VAR1: var value
            sources: [ path/to/source/file ]
            includes: [ path/to/include/dir ]
            pchs: [ path/to/pch ]
            dependencies: [ lib1, my::lib2 ]
            definitions:
              - DEFINE1: define value
              - DEFINE2
            properties:
              PROPERTY1: property value
            compile_options:
              - compile option
            link_options:
              - link option
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        // target
        CHECK(config.targets[0].type == cgen::config::TargetType::Executable);
        CHECK(config.targets[0].name == "executable name");
        // if
        CHECK(config.targets[0].if_ == "condition");
        // path
        CHECK(config.targets[0].executable.target_settings.path.value == "path/to/executable");
        // options
        CHECK(config.targets[0].executable.target_settings.options.size() == 1);
        CHECK(config.targets[0].executable.target_settings.options.at("OPTION1").description ==
              "option description");
        CHECK(config.targets[0].executable.target_settings.options.at("OPTION1").default_.value ==
              "default value");
        // settings
        CHECK(config.targets[0].executable.target_settings.settings.size() == 1);
        CHECK(config.targets[0].executable.target_settings.settings.at("VAR1").value ==
              "var value");
        // sources
        CHECK(config.targets[0].executable.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
        // includes
        CHECK(config.targets[0].executable.target_settings.includes.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.includes.private_.global[0].value ==
              "path/to/include/dir");
        // pchs
        CHECK(config.targets[0].executable.target_settings.pchs.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.pchs.private_.global[0].value ==
              "path/to/pch");
        // dependencies
        CHECK(config.targets[0].executable.target_settings.dependencies.private_.global.size() ==
              2);
        CHECK(config.targets[0].executable.target_settings.dependencies.private_.global[0].value ==
              "lib1");
        CHECK(config.targets[0].executable.target_settings.dependencies.private_.global[1].value ==
              "my::lib2");
        // definitions
        CHECK(config.targets[0].executable.target_settings.definitions.private_.global.size() == 2);
        CHECK(config.targets[0]
                  .executable.target_settings.definitions.private_.global[0]
                  .value.value.empty());
        CHECK(config.targets[0]
                  .executable.target_settings.definitions.private_.global[0]
                  .map.size() == 1);
        CHECK(config.targets[0]
                  .executable.target_settings.definitions.private_.global[0]
                  .map.at("DEFINE1")
                  .value == "define value");
        CHECK(config.targets[0]
                  .executable.target_settings.definitions.private_.global[1]
                  .value.value == "DEFINE2");
        CHECK(config.targets[0]
                  .executable.target_settings.definitions.private_.global[1]
                  .map.empty());
        // properties
        CHECK(config.targets[0].executable.target_settings.properties.global.size() == 1);
        CHECK(
            config.targets[0].executable.target_settings.properties.global.at("PROPERTY1").value ==
            "property value");
        // compile_options
        CHECK(config.targets[0].executable.target_settings.compile_options.private_.global.size() ==
              1);
        CHECK(
            config.targets[0].executable.target_settings.compile_options.private_.global[0].value ==
            "compile option");
        // link_options
        CHECK(config.targets[0].executable.target_settings.link_options.private_.global.size() ==
              1);
        CHECK(config.targets[0].executable.target_settings.link_options.private_.global[0].value ==
              "link option");
    }
}

TEST_CASE("visibility resolution") {
    SUBCASE("static library settings is private by default") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: static
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
    }

    SUBCASE("shared library settings is private by default") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: shared
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
    }

    SUBCASE("interface library settings has interface visibility by default") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            type: interface
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.interface.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.interface.global[0].value ==
              "path/to/source/file");
    }

    SUBCASE("executable settings is private by default") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - executable: executable name
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
    }
}

TEST_CASE("target configurations") {
    SUBCASE("default configuration") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - executable: executable name
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.empty() == false);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
    }

    SUBCASE("custom configurations") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - executable: executable name
            sources:
              configurations:
                Debug: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.empty() == false);
        CHECK(config.targets[0].executable.target_settings.sources.private_.configurations.size() ==
              1);
        CHECK(config.targets[0]
                  .executable.target_settings.sources.private_.configurations.at("Debug")
                  .size() == 1);
        CHECK(config.targets[0]
                  .executable.target_settings.sources.private_.configurations.at("Debug")[0]
                  .value == "path/to/source/file");
    }

    SUBCASE("default configuration with custom configurations") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - executable: executable name
            sources:
              global: [ default/path/to/source/file ]
              configurations:
                Debug: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].executable.target_settings.sources.private_.global[0].value ==
              "default/path/to/source/file");
        CHECK(config.targets[0].executable.target_settings.sources.private_.configurations.size() ==
              1);
        CHECK(config.targets[0]
                  .executable.target_settings.sources.private_.configurations.at("Debug")
                  .size() == 1);
        CHECK(config.targets[0]
                  .executable.target_settings.sources.private_.configurations.at("Debug")[0]
                  .value == "path/to/source/file");
    }

    SUBCASE("visibility with configurations") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            sources:
              private: [ private/path/to/source/file ]
              public:
                global: [ default/path/to/source/file ]
                configurations:
                  Debug: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "private/path/to/source/file");
        CHECK(config.targets[0].library.target_settings.sources.public_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.public_.global[0].value ==
              "default/path/to/source/file");
        CHECK(config.targets[0].library.target_settings.sources.public_.configurations.size() == 1);
        CHECK(config.targets[0]
                  .library.target_settings.sources.public_.configurations.at("Debug")
                  .size() == 1);
        CHECK(config.targets[0]
                  .library.target_settings.sources.public_.configurations.at("Debug")[0]
                  .value == "path/to/source/file");
    }
}

TEST_CASE("includes mering") {
    SUBCASE("merge includes without parameters") {
        cgen::mock_files({
            {"path1", R"(
            settings:
              VAR1: included value
            targets:
              - library: included library 1
              - library: included library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
        settings:
          VAR1: original value
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.settings.size() == 1);
        CHECK(config.settings.at("VAR1").value == "included value");
        CHECK(config.targets.size() == 3);
        CHECK(config.targets[0].name == "library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "included library 2");
    }

    SUBCASE("merge includes with shared parameters") {
        cgen::mock_files({
            {"path1", R"(
            targets:
              - library: $(library) library 1
            )"},
            {"path2", R"(
            targets:
              - library: $(library) library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - paths: [ path1, path2 ]
            parameters:
              library: included
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 3);
        CHECK(config.targets[0].name == "library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "included library 2");
    }

    SUBCASE("merge includes with independent parameters") {
        cgen::mock_files({
            {"path1", R"(
            targets:
              - library: $(library) library 1
            )"},
            {"path2", R"(
            targets:
              - library: $(library) library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - paths: [ path1 ]
            parameters:
              library: included
          - paths: [ path2 ]
            parameters:
              library: another
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 3);
        CHECK(config.targets[0].name == "library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "another library 2");
    }

    SUBCASE("merge includes keeping original targets") {
        cgen::mock_files({
            {"path1", R"(
            settings:
              VAR1: included value
            targets:
              - library: included library 1
              - library: included library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
        targets:REPLACE:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].name == "library");
    }

    SUBCASE("merge includes replacing targets in second include") {
        cgen::mock_files({
            {"path1", R"(
            settings:
              VAR1: included value
            targets:
              - library: included library 1
              - library: included library 2
            )"},
            {"path2", R"(
            targets:REPLACE:
              - library: included library
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
          - path2
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].name == "included library");
    }

    SUBCASE("merge includes replacing targets in first include") {
        cgen::mock_files({
            {"path1", R"(
            settings:
              VAR1: included value
            targets:
              - library: included library 1
              - library: included library 2
            )"},
            {"path2", R"(
            targets:REPLACE:
              - library: included library
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path2
          - path1
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 3);
        CHECK(config.targets[0].name == "included library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "included library 2");
    }

    SUBCASE("merge nested includes without parameters") {
        cgen::mock_files({
            {"path1", R"(
            includes:
              - nested1
              - nested2
            targets:
              - library: included library 1
            )"},
            {"nested1", R"(
            targets:
              - library: nested library 1
            )"},
            {"nested2", R"(
            targets:
              - library: nested library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 4);
        CHECK(config.targets[0].name == "library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "nested library 1");
        CHECK(config.targets[3].name == "nested library 2");
    }

    SUBCASE("merge nested includes with parameters") {
        cgen::mock_files({
            {"path1", R"(
            includes:
              - paths: [ $(nested) ]
                parameters:
                  library: nested
            targets:
              - library: $(library) library 1
            )"},
            {"nested1", R"(
            targets:
              - library: $(library) library 1
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - paths: [ path1 ]
            parameters:
              nested: nested1
              library: included
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 3);
        CHECK(config.targets[0].name == "library");
        CHECK(config.targets[1].name == "included library 1");
        CHECK(config.targets[2].name == "nested library 1");
    }

    SUBCASE("don't pass parameters to the nested includes") {
        cgen::mock_files({
            {"path1", R"(
            includes:
              - nested1
            targets:
              - library: $(library) library 1
            )"},
            {"nested1", R"(
            targets:
              - library: $(library) library 1
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - paths: [ path1 ]
            parameters:
              library: included
        targets:
          - library: library
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "nested1: undefined config include parameter: library");
    }

    SUBCASE("always keep version, project and includes") {
        cgen::mock_files({
            {"path1", R"(
            version: 0
            project:
              version: 0
              name: included project name
            includes:
              - path2
            )"},
            {"path2", ""},
        });

        std::istringstream config_iss{R"(
        version: 1
        project: project name
        includes:
          - path1
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.version == "1");
        CHECK(config.project.name == "project name");
        CHECK(config.includes.size() == 1);
        CHECK(config.includes[0].paths.size() == 1);
        CHECK(config.includes[0].paths[0] == "path1");
    }

    SUBCASE("ignore recursive includes") {
        cgen::mock_files({
            {"path1", R"(
            includes:
              - path2
            )"},
            {"path2", R"(
            includes:
              - path1
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
    }

    SUBCASE("error if include not found") {
        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path999
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "config include file not found: path999");
    }

    SUBCASE("error if include parameter not found") {
        cgen::mock_files({
            {"path1", R"(
            targets:
              - library: $(library) library 1
            )"},
            {"path2", R"(
            targets:
              - library: $(library) library 2
            )"},
        });

        std::istringstream config_iss{R"(
        project: project name
        includes:
          - path1
          - path2
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 2);
        CHECK(errors[0].description() == "path1: undefined config include parameter: library");
        CHECK(errors[1].description() == "path2: undefined config include parameter: library");
    }
}

TEST_CASE("target templates merging") {
    SUBCASE("merge target templates without parameters") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ path/to/source/file1 ]
          template2:
            sources: [ path/to/source/file2 ]
        targets:
          - library: library name
            templates: [ template1, template2 ]
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 3);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[1].value ==
              "path/to/source/file1");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[2].value ==
              "path/to/source/file2");
    }

    SUBCASE("merge target templates with shared parameters") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ $(path)/file1 ]
          template2:
            sources: [ $(path)/file2 ]
        targets:
          - library: library name
            templates:
              - names: [ template1, template2 ]
                parameters:
                  path: path/to/source
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 3);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[1].value ==
              "path/to/source/file1");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[2].value ==
              "path/to/source/file2");
    }

    SUBCASE("merge target templates with independent parameters") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ $(path)/file1 ]
          template2:
            sources: [ $(path)/file2 ]
          template3:
            sources: [ $(path)/file3 ]
        targets:
          - library: library name
            templates:
              - names: [ template1, template2 ]
                parameters:
                  path: path/to/source
              - names: [ template3 ]
                parameters:
                  path: my/path/to/source
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 4);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[1].value ==
              "path/to/source/file1");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[2].value ==
              "path/to/source/file2");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[3].value ==
              "my/path/to/source/file3");
    }

    SUBCASE("merge target templates keeping original sources") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ path/to/source/file1 ]
          template2:
            sources: [ path/to/source/file2 ]
        targets:
          - library: library name
            templates: [ template1, template2 ]
            sources:REPLACE: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file");
    }

    SUBCASE("merge target templates replacing original sources in first template") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources:REPLACE: [ path/to/source/file1 ]
          template2:
            sources: [ path/to/source/file2 ]
        targets:
          - library: library name
            templates: [ template1, template2 ]
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 2);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file1");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[1].value ==
              "path/to/source/file2");
    }

    SUBCASE("merge target templates replacing original sources in second template") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ path/to/source/file1 ]
          template2:
            sources:REPLACE: [ path/to/source/file2 ]
        targets:
          - library: library name
            templates: [ template1, template2 ]
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file2");
    }

    SUBCASE("merge target templates with different structure") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources:
              public: [ path/to/source/file1 ]
          template2:
            sources:
              public:
                configurations:
                  Release: [ path/to/source/file2 ]
          template3:
            sources:
              configurations:
                Release: [ path/to/source/file3 ]
          template4:
            sources:
              configurations:REPLACE:
                Release: [ path/to/source/file4 ]
          template5:
            sources:
              private: [ path/to/source/file5 ]
        targets:
          - library: library name
            templates:
              - template1
              - template2
              - template3
              - template4
              - template5
            sources: [ path/to/source/file ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.empty());
        CHECK(config.targets.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.public_.global.size() == 1);
        CHECK(config.targets[0].library.target_settings.sources.public_.global[0].value ==
              "path/to/source/file1");
        CHECK(config.targets[0]
                  .library.target_settings.sources.public_.configurations.at("Release")
                  .size() == 1);
        CHECK(config.targets[0]
                  .library.target_settings.sources.public_.configurations.at("Release")[0]
                  .value == "path/to/source/file2");
        CHECK(config.targets[0]
                  .library.target_settings.sources.private_.configurations.at("Release")
                  .size() == 1);
        CHECK(config.targets[0]
                  .library.target_settings.sources.private_.configurations.at("Release")[0]
                  .value == "path/to/source/file4");
        CHECK(config.targets[0].library.target_settings.sources.private_.global.size() == 2);
        CHECK(config.targets[0].library.target_settings.sources.private_.global[0].value ==
              "path/to/source/file5");
        CHECK(config.targets[0].library.target_settings.sources.private_.global[1].value ==
              "path/to/source/file");
    }

    SUBCASE("error if template not found") {
        std::istringstream config_iss{R"(
        project: project name
        targets:
          - library: library name
            templates: [ template1, template2 ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 2);
        CHECK(errors[0].description() == "library name: config template not found: template1");
        CHECK(errors[1].description() == "library name: config template not found: template2");
    }

    SUBCASE("error if template parameter not found") {
        std::istringstream config_iss{R"(
        project: project name
        templates:
          template1:
            sources: [ $(path)/file1 ]
        targets:
          - library: library name
            templates: [ template1 ]
        )"};

        std::vector<cgen::Error> errors;
        const cgen::Config config = config_read(config_iss, errors);
        CHECK(errors.size() == 1);
        CHECK(errors[0].description() == "template1: undefined config template parameter: path");
    }
}
