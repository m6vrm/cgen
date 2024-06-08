#include "codegen.hpp"
#include "config.hpp"
#include "version.hpp"

#include <doctest/doctest.h>

#include <sstream>

inline const std::string CMAKE_LISTS_HEADER = R"(
# Generated using cgen 1.1.1 — https://github.com/m6vrm/cgen
# DO NOT EDIT

cmake_minimum_required(VERSION 3.11)
)";

auto config_generate(std::istream &in) -> std::string {
    std::vector<cgen::Error> errors;
    const cgen::Config config =
        cgen::config_read(in, cgen::version::major, errors);
    CHECK(errors.empty());

    std::ostringstream oss;
    cgen::CMakeGenerator cmake{oss};
    cmake.write(config);

    std::string str = "\n" + oss.str();
    str.replace(str.find(CMAKE_LISTS_HEADER), CMAKE_LISTS_HEADER.size(), "\n");
    return str;
}

TEST_CASE("project generation") {
    SUBCASE("project without version") {
        std::istringstream config_iss{"project: name"};
        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)
)");
    }

    SUBCASE("project with version") {
        std::istringstream config_iss{R"(
        project:
          name: name
          version: 1.0
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name VERSION 1.0)
)");
    }
}

TEST_CASE("options generation") {
    SUBCASE("options without default value") {
        std::istringstream config_iss{R"(
        project: name
        options:
          OPTION1:
            description: Option 1
          OPTION2:
            description: Option 2
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Options
#

option(OPTION1 "Option 1")
option(OPTION2 "Option 2")
)");
    }

    SUBCASE("options with default value") {
        std::istringstream config_iss{R"(
        project: name
        options:
          OPTION1:
            description: Option 1
            default: Value
          OPTION2:
            description: Option 2
            default: "Quoted"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Options
#

option(OPTION1 "Option 1" Value)
option(OPTION2 "Option 2" "Quoted")
)");
    }
}

TEST_CASE("settings generation") {
    std::istringstream config_iss{R"(
    project: name
    settings:
        VAR1: Value
        VAR2: "Quoted"
    )"};

    const std::string cmake = config_generate(config_iss);
    CHECK(cmake == R"(
project(name)

#
# Settings
#

set(VAR1 Value)
set(VAR2 "Quoted")
)");
}

TEST_CASE("packages generation") {
    SUBCASE("system packages") {
        std::istringstream config_iss{R"(
        project: name
        packages:
          - system: Package1
          - system: Package2
            if: condition
            version: 1.0.0
            required: false
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# System packages
#

find_package(Package1 REQUIRED)
if(condition)
    find_package(Package2 1.0.0)
endif()
)");
    }

    SUBCASE("external packages") {
        std::istringstream config_iss{R"(
        project: name
        packages:
          - external: Package1
            url: https://external.com/repo.git
          - external: Package2
            if: condition
            url: https://external.com/repo.git
            version: 1.0.0
            options:
              OPTION1: Value
              OPTION2: "Quoted"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# External packages
#

# package Package1
function(cgen_package_0)
    if(EXISTS ${PROJECT_SOURCE_DIR}/Package1/CMakeLists.txt)
        add_subdirectory(Package1)
    else()
        message(NOTICE "Package Package1 doesn't have CMakeLists.txt")
    endif()
endfunction()
cgen_package_0()

# package Package2
function(cgen_package_1)
    set(OPTION1 Value CACHE INTERNAL "" FORCE)
    set(OPTION2 "Quoted" CACHE INTERNAL "" FORCE)
    if(EXISTS ${PROJECT_SOURCE_DIR}/Package2/CMakeLists.txt)
        add_subdirectory(Package2)
    else()
        message(NOTICE "Package Package2 doesn't have CMakeLists.txt")
    endif()
endfunction()
if(condition)
    cgen_package_1()
endif()
)");
    }
}

TEST_CASE("target settings generation") {
    SUBCASE("target options") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            options:
              OPTION1:
                description: Description
                default: Value
              OPTION2:
                description: Description
                default: "Quoted"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Target options
#

# options for target library
option(OPTION1 "Description" Value)
option(OPTION2 "Description" "Quoted")

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library STATIC)
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("target settings") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            settings:
              VAR1: Value
              VAR2: "Quoted"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    set(VAR1 Value)
    set(VAR2 "Quoted")
    add_library(library STATIC)
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("target sources") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            sources:
              - path/to/file
          - library: library2
            sources:
              public:
                - path/to/file
          - library: library3
            sources:
              public:
                global:
                  - path/to/file
                configurations:
                  Release:
                    - "path/to/file"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_sources(library1
        PRIVATE
            path/to/file
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_sources(library2
        PUBLIC
            path/to/file
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_sources(library3
        PUBLIC
            path/to/file
            $<$<CONFIG:Release>:
                "path/to/file"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target includes") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            includes:
              - path/to/file
          - library: library2
            includes:
              public:
                - path/to/file
          - library: library3
            includes:
              public:
                global:
                  - path/to/file
                configurations:
                  Release:
                    - "path/to/file"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_include_directories(library1
        PRIVATE
            path/to/file
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_include_directories(library2
        PUBLIC
            path/to/file
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_include_directories(library3
        PUBLIC
            path/to/file
            $<$<CONFIG:Release>:
                "path/to/file"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target pchs") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            pchs:
              - path/to/file
          - library: library2
            pchs:
              public:
                - path/to/file
          - library: library3
            pchs:
              public:
                global:
                  - path/to/file
                configurations:
                  Release:
                    - "path/to/file"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_precompiled_headers(library1
        PRIVATE
            path/to/file
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_precompiled_headers(library2
        PUBLIC
            path/to/file
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_precompiled_headers(library3
        PUBLIC
            path/to/file
            $<$<CONFIG:Release>:
                "path/to/file"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target dependencies") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            dependencies:
              - dependency
          - library: library2
            dependencies:
              public:
                - dependency
          - library: library3
            dependencies:
              public:
                global:
                  - dependency
                configurations:
                  Release:
                    - "dependency"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_link_libraries(library1
        PRIVATE
            dependency
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_link_libraries(library2
        PUBLIC
            dependency
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_link_libraries(library3
        PUBLIC
            dependency
            $<$<CONFIG:Release>:
                "dependency"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target definitions") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            definitions:
              - DEFINITION
              - KEY: VALUE
          - library: library2
            definitions:
              public:
                - DEFINITION
                - KEY: VALUE
          - library: library3
            definitions:
              public:
                global:
                  - DEFINITION
                  - KEY: VALUE
                configurations:
                  Release:
                    - "DEFINITION"
                    - KEY: "VALUE"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_compile_definitions(library1
        PRIVATE
            DEFINITION
            KEY=VALUE
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_compile_definitions(library2
        PUBLIC
            DEFINITION
            KEY=VALUE
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_compile_definitions(library3
        PUBLIC
            DEFINITION
            KEY=VALUE
            $<$<CONFIG:Release>:
                "DEFINITION"
                KEY="VALUE"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target properties") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            properties:
              KEY: VALUE
          - library: library2
            properties:
              KEY: VALUE
          - library: library3
            properties:
              global:
                KEY: VALUE
              configurations:
                Release:
                  KEY: "VALUE"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    set_target_properties(library1 PROPERTIES
        KEY VALUE
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    set_target_properties(library2 PROPERTIES
        KEY VALUE
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    set_target_properties(library3 PROPERTIES
        KEY VALUE
        $<$<CONFIG:Release>:
            KEY "VALUE"
        >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target compile options") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            compile_options:
              - option
          - library: library2
            compile_options:
              public:
                - option
          - library: library3
            compile_options:
              public:
                global:
                  - option
                configurations:
                  Release:
                    - "option"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_compile_options(library1
        PRIVATE
            option
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_compile_options(library2
        PUBLIC
            option
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_compile_options(library3
        PUBLIC
            option
            $<$<CONFIG:Release>:
                "option"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target link options") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            link_options:
              - option
          - library: library2
            link_options:
              public:
                - option
          - library: library3
            link_options:
              public:
                global:
                  - option
                configurations:
                  Release:
                    - "option"
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
    target_link_options(library1
        PRIVATE
            option
    )
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
    target_link_options(library2
        PUBLIC
            option
    )
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
    target_link_options(library3
        PUBLIC
            option
            $<$<CONFIG:Release>:
                "option"
            >
    )
endfunction()
cgen_target_2()
)");
    }

    SUBCASE("target path") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            path: prefix
            sources:
              - path/to/file
            includes:
              - path/to/file
            pchs:
              - path/to/file
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library STATIC)
    target_sources(library
        PRIVATE
            prefix/path/to/file
    )
    target_include_directories(library
        PRIVATE
            prefix/path/to/file
    )
    target_precompiled_headers(library
        PRIVATE
            prefix/path/to/file
    )
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("empty target settings") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library1
            sources: []
          - library: library2
            sources:
              public: []
          - library: library3
            sources:
              public:
                global: []
                configurations:
                  Release: []
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library1
function(cgen_target_0)
    add_library(library1 STATIC)
endfunction()
cgen_target_0()

# target library2
function(cgen_target_1)
    add_library(library2 STATIC)
endfunction()
cgen_target_1()

# target library3
function(cgen_target_2)
    add_library(library3 STATIC)
endfunction()
cgen_target_2()
)");
    }
}

TEST_CASE("target generation") {
    SUBCASE("executable target") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - executable: executable
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target executable
function(cgen_target_0)
    add_executable(executable)
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("static library target") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            type: static
            if: condition
            aliases:
              - alias1
              - alias2
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library STATIC)
    add_library(alias1 ALIAS library)
    add_library(alias2 ALIAS library)
endfunction()
if(condition)
    cgen_target_0()
endif()
)");
    }

    SUBCASE("shared library target") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            type: shared
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library SHARED)
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("interface library target") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            type: interface
            sources:
              - path/to/file
            includes:
              - path/to/file
            pchs:
              - path/to/file
            dependencies:
              - dependency
            definitions:
              - DEFINITION
            properties:
              KEY: VALUE
            compile_options:
              - option
            link_options:
              - option
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library INTERFACE)
    target_sources(library
        INTERFACE
            path/to/file
    )
    target_include_directories(library
        INTERFACE
            path/to/file
    )
    target_precompiled_headers(library
        INTERFACE
            path/to/file
    )
    target_link_libraries(library
        INTERFACE
            dependency
    )
    target_compile_definitions(library
        INTERFACE
            DEFINITION
    )
    set_target_properties(library PROPERTIES
        KEY VALUE
    )
    target_compile_options(library
        INTERFACE
            option
    )
    target_link_options(library
        INTERFACE
            option
    )
endfunction()
cgen_target_0()
)");
    }

    SUBCASE("object library target") {
        std::istringstream config_iss{R"(
        project: name
        targets:
          - library: library
            type: object
        )"};

        const std::string cmake = config_generate(config_iss);
        CHECK(cmake == R"(
project(name)

#
# Targets
#

# target library
function(cgen_target_0)
    add_library(library OBJECT)
endfunction()
cgen_target_0()
)");
    }
}
