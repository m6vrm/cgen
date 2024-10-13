# Generated using cgen 1.1.1 — https://github.com/m6vrm/cgen
# DO NOT EDIT

cmake_minimum_required(VERSION 3.11)
project(cgen)

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

# package external/yaml-cpp
function(cgen_package_1)
    set(YAML_BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE INTERNAL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "" FORCE)
    set(YAML_CPP_FORMAT_SOURCE OFF CACHE INTERNAL "" FORCE)
    if(EXISTS ${PROJECT_SOURCE_DIR}/external/yaml-cpp/CMakeLists.txt)
        add_subdirectory(external/yaml-cpp)
    else()
        message(NOTICE "Package external/yaml-cpp doesn't have CMakeLists.txt")
    endif()
endfunction()
cgen_package_1()

# package external/miroir
function(cgen_package_2)
    if(EXISTS ${PROJECT_SOURCE_DIR}/external/miroir/CMakeLists.txt)
        add_subdirectory(external/miroir)
    else()
        message(NOTICE "Package external/miroir doesn't have CMakeLists.txt")
    endif()
endfunction()
cgen_package_2()

# package external/poost
function(cgen_package_3)
    if(EXISTS ${PROJECT_SOURCE_DIR}/external/poost/CMakeLists.txt)
        add_subdirectory(external/poost)
    else()
        message(NOTICE "Package external/poost doesn't have CMakeLists.txt")
    endif()
endfunction()
cgen_package_3()

#
# Targets
#

# target libcgen_obj
function(cgen_target_0)
    add_library(libcgen_obj OBJECT)
    target_sources(libcgen_obj
        PRIVATE
            src/libcgen/codegen.cpp
            src/libcgen/config.cpp
            src/libcgen/debug.cpp
            src/libcgen/error.cpp
            src/libcgen/fs.cpp
            src/libcgen/git.cpp
            src/libcgen/packages.cpp
            src/libcgen/preproc.cpp
            src/libcgen/version.cpp
            src/libcgen/versions.cpp
    )
    target_include_directories(libcgen_obj
        PUBLIC
            include
        PRIVATE
            include/libcgen
            src/libcgen
    )
    target_link_libraries(libcgen_obj
        PUBLIC
            yaml-cpp
        PRIVATE
            miroir::miroir
            poost::poost
    )
    set_target_properties(libcgen_obj PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
    target_compile_options(libcgen_obj
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

# target libcgen
function(cgen_target_1)
    add_library(libcgen STATIC)
    add_library(cgen::libcgen ALIAS libcgen)
    target_sources(libcgen
        PRIVATE
            src/libcgen/exec_impl.cpp
            src/libcgen/fs_impl.cpp
    )
    target_include_directories(libcgen
        PRIVATE
            src/libcgen
    )
    target_link_libraries(libcgen
        PUBLIC
            libcgen_obj
        PRIVATE
            poost::poost
    )
    set_target_properties(libcgen PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
    target_compile_options(libcgen
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:
                -Werror
            >
    )
endfunction()
cgen_target_1()

# target libcgen_test
function(cgen_target_2)
    add_executable(libcgen_test)
    target_sources(libcgen_test
        PRIVATE
            tests/codegen_test.cpp
            tests/config_test.cpp
            tests/exec_mock.cpp
            tests/fs_mock.cpp
            tests/fs_test.cpp
            tests/main_test.cpp
            tests/packages_test.cpp
            tests/preproc_test.cpp
            tests/versions_test.cpp
    )
    target_include_directories(libcgen_test
        PRIVATE
            include/libcgen
            src/libcgen
            tests
    )
    target_link_libraries(libcgen_test
        PRIVATE
            doctest::doctest
            libcgen_obj
            poost::poost
    )
    set_target_properties(libcgen_test PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
    target_compile_options(libcgen_test
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
    target_link_options(libcgen_test
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
    cgen_target_2()
endif()

# target cgen
function(cgen_target_3)
    add_executable(cgen)
    target_sources(cgen
        PRIVATE
            src/cgen/main.cpp
    )
    target_include_directories(cgen
        PRIVATE
            src/cgen
    )
    target_link_libraries(cgen
        PRIVATE
            cgen::libcgen
            poost::poost
    )
    set_target_properties(cgen PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
    )
    target_compile_options(cgen
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            $<$<CONFIG:Release>:
                -Werror
            >
    )
endfunction()
cgen_target_3()
