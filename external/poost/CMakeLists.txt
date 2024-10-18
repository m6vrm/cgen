# Generated using cgen 1.1.2 — https://github.com/m6vrm/cgen
# DO NOT EDIT

cmake_minimum_required(VERSION 3.11)
project(poost)

#
# External packages
#

# package external/doctest
function(cgen_package_0)
    set(DOCTEST_NO_INSTALL ON CACHE INTERNAL "" FORCE)
    if(EXISTS ${PROJECT_SOURCE_DIR}/external/doctest/CMakeLists.txt)
        add_subdirectory(external/doctest)
    else()
        message(NOTICE "Package external/doctest doesn't have CMakeLists.txt")
    endif()
endfunction()
if(PROJECT_IS_TOP_LEVEL)
    cgen_package_0()
endif()

# package external/fmt
function(cgen_package_1)
    set(FMT_INSTALL OFF CACHE INTERNAL "" FORCE)
    if(EXISTS ${PROJECT_SOURCE_DIR}/external/fmt/CMakeLists.txt)
        add_subdirectory(external/fmt)
    else()
        message(NOTICE "Package external/fmt doesn't have CMakeLists.txt")
    endif()
endfunction()
cgen_package_1()

#
# Targets
#

# target poost
function(cgen_target_0)
    add_library(poost STATIC)
    add_library(poost::poost ALIAS poost)
    target_sources(poost
        PRIVATE
            include/poost/args.hpp
            include/poost/assert.hpp
            include/poost/config.hpp
            include/poost/format.hpp
            include/poost/log.hpp
            src/poost/args.cpp
            src/poost/config.cpp
            src/poost/log.cpp
    )
    target_include_directories(poost
        PUBLIC
            include
        PRIVATE
            include/poost
    )
    target_link_libraries(poost
        PUBLIC
            fmt::fmt
    )
    set_target_properties(poost PROPERTIES
        CXX_STANDARD 23
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
cgen_target_0()

# target poost_test
function(cgen_target_1)
    add_executable(poost_test)
    target_sources(poost_test
        PRIVATE
            tests/args_test.cpp
            tests/assert_test.cpp
            tests/config_test.cpp
            tests/log_test.cpp
    )
    target_include_directories(poost_test
        PRIVATE
            include/poost
            src/poost
    )
    target_link_libraries(poost_test
        PRIVATE
            doctest::doctest_with_main
            poost::poost
    )
    set_target_properties(poost_test PROPERTIES
        CXX_STANDARD 23
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
    cgen_target_1()
endif()
