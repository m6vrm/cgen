# Configuration specification

[[_TOC_]]

- [Configuration example](../.cgen.yml)
- [Generated CMake configuration](../CMakeLists.txt)
- [Full configuration schema](../src/libcgen/cgen.schema.yml.in)
- [How to read schema file](https://gitlab.com/madyanov/miroir/-/blob/master/README.md#schema-specification)

## Project

Project name and version.

**Schema**

```yml
project:
  - string                      # project name
  - name: string                # project name
    version: !optional scalar   # version
```

**Examples**

```yml
project: project_name
```

```yml
project:
  name: project_name
  version: 0.1.0
```

**Generated configuration**

```cmake
project(project_name VERSION 0.1.0)
```

## Options

CMake options.

**Schema**

```yml
options: !optional
  $string:                      # option name
    description: string         # description
    default: !optional string   # default value (default: NO)
```

**Examples**

```yml
options:
  MY_OPTION:
    description: "My option"
    default: "default value"
```

**Generated configuration**

```cmake
option(MY_OPTION "My option" "default value")
```

## Settings

CMake variables.

**Schema**

```yml
settings: !optional
  $string: scalar
```

**Examples**

```yml
settings:
  MY_VAR1: "some value"
  MY_VAR2: 42
```

**Generated configuration**

```cmake
set(MY_VAR1 "some value")
set(MY_VAR2 42)
```

## Packages

System and external dependencies.

**Schema**

```yml
packages: !optional
  - - external: string                      # unique path to the package
      if: !optional string                  # condition (any CMake expression)
      url: string                           # url to the package repository
      version: !optional scalar             # package version, default: HEAD
      strategy: !optional !variant          # fetch strategy, default: submodule
        - submodule                         # fetch as git submodule
        - clone                             # fetch as raw files
      options: !optional                    # CMake option overrides for the package
        $string: scalar

    - system: string                # name of the system CMake package
      if: !optional string          # condition
      version: !optional scalar     # package version
      required: !optional boolean   # default: true
```

**Examples**

```yml
packages:
  - external: external/doctest
    if: PROJECT_IS_TOP_LEVEL
    url: https://github.com/doctest/doctest
    options:
      DOCTEST_NO_INSTALL: ON

  - system: OpenGL
```

**Generated configuration**

```cmake
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
```

## Targets

Project targets.

**Schema**

```yml
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
          parameters: !optional     # template parameters, e.g. $(MY_PARAMETER)
            $string: string
    ...                             # target settings, see below

  - executable: string              # executable target name
    templates: !optional            # target templates, same as above
      ...
    ...                             # target settings, see below
```

Target settings schema:

```yml
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
```

**Examples**

```yml

```
