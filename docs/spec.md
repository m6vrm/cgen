Configuration specification
===========================

Project
-------

Project name and version.

Schema:

    project:
      - string                    # project name
      - name: string              # project name
        version: !optional scalar # version

Example:

    project: project_name

    project:
      name: project_name
      version: 0.1.0

Generated configuration:

    project(project_name VERSION 0.1.0)

Options
-------

CMake options.

Schema:

    options: !optional
      $string:                      # option name
        description: string         # description
        default: !optional string   # default value (default: NO)

Example:

    options:
      MY_OPTION:
        description: "My option"
        default: "default value"

Generated configuration:

    option(MY_OPTION "My option" "default value")

Settings
--------

CMake variables.

Schema:

    settings: !optional
      $string: scalar

Example:

    settings:
      MY_VAR1: "some value"
      MY_VAR2: 42

Generated configuration:

    set(MY_VAR1 "some value")
    set(MY_VAR2 42)

Packages
--------

System and external dependencies.

Schema:

    packages: !optional
      - - external: string              # unique path to the package
          if: !optional string          # condition (any CMake expression)
          url: string                   # url to the package repository
          version: !optional scalar     # package version, default: HEAD
          strategy: !optional !variant  # fetch strategy, default: submodule
            - submodule                 # fetch as git submodule
            - clone                     # fetch as raw files
          options: !optional            # CMake option overrides for the package
            $string: scalar

        - system: string                # name of the system CMake package
          if: !optional string          # condition
          version: !optional scalar     # package version
          required: !optional boolean   # default: true

Example:

    packages:
      - external: external/doctest
        if: PROJECT_IS_TOP_LEVEL
        url: https://github.com/doctest/doctest
        options:
          DOCTEST_NO_INSTALL: ON

      - system: OpenGL

Generated configuration:

    function(cgen_package_0)
        set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "" FORCE)
        if(EXISTS ${PROJECT_SOURCE_DIR}/external/doctest/CMakeLists.txt)
            add_subdirectory(external/doctest)
        else()
            message(WARNING "Package external/doctest doesn't have CMakeLists.txt")
        endif()
    endfunction()
    if(PROJECT_IS_TOP_LEVEL)
        cgen_package_0()
    endif()

    find_package(OpenGL REQUIRED)

Targets
-------

Project targets.

Schema:

    targets: !optional
      - library: string                 # library target name
        type: !optional !variant        # library type, default: static
          - static
          - shared
          - interface
          - object
        aliases: !optional [string]     # CMake library aliases
        templates: !optional            # target templates
          - [string]                    # template names
          - - names: [string]           # template names
              parameters: !optional     # template parameters,
                                        # e.g. $(MY_PARAMETER)
                $string: string
        ...                             # target settings, see below

      - executable: string              # executable target name
        templates: !optional            # target templates, same as above
          ...
        ...                             # target settings, see below

Common target settings schema:

    if: !optional string            # condition (any CMake expression)
    path: !optional string          # prefix path to the target sources
    options: !optional              # target options
      $string:
        description: string
        default: !optional string
    settings: !optional             # target variables
      $string: scalar
    sources: !optional              # source files
    includes: !optional             # include paths
    pchs: !optional                 # precompiled headers
    dependencies: !optional         # dependency library names
    definitions: !optional          # preprocessor definitions
    properties: !optional           # target properties
    compile_options: !optional      # compile options
    link_options: !optional         # link options

Example:

    templates:
      common:
        properties:
          CXX_STANDARD: 20
          CXX_STANDARD_REQUIRED: ON
        compile_options:
          global:
            - -Wall
            - -Wextra
            - -Wpedantic
          configurations:
            Release:
              - -Werror

      asan:
        compile_options:
          configurations:
            Asan:
              - ${CMAKE_CXX_FLAGS_DEBUG}
              - -O1
              - -fno-omit-frame-pointer
              - -fno-optimize-sibling-calls
              - -fsanitize=address
        link_options:
          configurations:
            Asan:
              - ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
              - -g
              - -fsanitize=address

      ubsan:
        compile_options:
          configurations:
            Ubsan:
              - ${CMAKE_CXX_FLAGS_DEBUG}
              - -O1
              - -fno-omit-frame-pointer
              - -fno-optimize-sibling-calls
              - -fsanitize=undefined
              - -fno-sanitize-recover
        link_options:
          configurations:
            Ubsan:
              - ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
              - -g
              - -fsanitize=undefined

    targets:
      - library: poost
        templates:
          - common
        aliases:
          - poost::poost
        sources:
          - include/poost/args.hpp
          - src/poost/args.cpp

          - include/poost/log.hpp
          - src/poost/log.cpp

          - include/poost/assert.hpp
        includes:
          public:
            - include
          private:
            - include/poost

      - executable: poost_test
        if: PROJECT_IS_TOP_LEVEL
        templates:
          - common
          - asan
          - ubsan
        sources:
          - tests/args_test.cpp
        includes:
          - src/poost
        dependencies:
          - doctest::doctest_with_main
          - poost::poost

Generated configuration:

    function(cgen_target_poost)
        add_library(poost STATIC)
        add_library(poost::poost ALIAS poost)
        target_sources(poost
            PRIVATE
                include/poost/args.hpp
                src/poost/args.cpp
                include/poost/log.hpp
                src/poost/log.cpp
                include/poost/assert.hpp
        )
        target_include_directories(poost
            PUBLIC
                include
            PRIVATE
                include/poost
        )
        set_target_properties(poost PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
        )
        target_compile_options(poost
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                $<$<CONFIG:Release>:
                    -Werror
                >
        )
    endfunction()
    cgen_target_poost()

    function(cgen_target_poost_test)
        add_executable(poost_test)
        target_sources(poost_test
            PRIVATE
                tests/args_test.cpp
        )
        target_include_directories(poost_test
            PRIVATE
                src/poost
        )
        target_link_libraries(poost_test
            PRIVATE
                doctest::doctest_with_main
                poost::poost
        )
        set_target_properties(poost_test PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
        )
        target_compile_options(poost_test
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                $<$<CONFIG:Asan>:
                    ${CMAKE_CXX_FLAGS_DEBUG}
                    -O1
                    -fno-omit-frame-pointer
                    -fno-optimize-sibling-calls
                    -fsanitize=address
                >
                $<$<CONFIG:Release>:
                    -Werror
                >
                $<$<CONFIG:Ubsan>:
                    ${CMAKE_CXX_FLAGS_DEBUG}
                    -O1
                    -fno-omit-frame-pointer
                    -fno-optimize-sibling-calls
                    -fsanitize=undefined
                    -fno-sanitize-recover
                >
        )
        target_link_options(poost_test
            PRIVATE
                $<$<CONFIG:Asan>:
                    ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
                    -g
                    -fsanitize=address
                >
                $<$<CONFIG:Ubsan>:
                    ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
                    -g
                    -fsanitize=undefined
                >
        )
    endfunction()
    if(PROJECT_IS_TOP_LEVEL)
        cgen_target_poost_test()
    endif()
