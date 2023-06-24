# Configuration specification

[[_TOC_]]

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
    templates: !optional templates  # target templates, see below
    ...                             # target settings, see below

  - executable: string              # executable target name
    templates: !optional templates  # target templates, see below
    ...                             # target settings, see below
```

Target settings schema:

```yml
if: !optional string
path: !optional string
options: !optional
  $string:                      # option name
    description: string         # description
    default: !optional string   # default value (default: NO)
settings: !optional
  $string: scalar
sources: !optional visibility<configurations<list<string>>>
includes: !optional visibility<configurations<list<string>>>
pchs: !optional visibility<configurations<list<string>>>
dependencies: !optional visibility<configurations<list<string>>>
definitions: !optional visibility<configurations<list<definition>>>
properties: !optional configurations<map<string;scalar>>
compile_options: !optional visibility<configurations<list<string>>>
link_options: !optional visibility<configurations<list<string>>>
```

**Examples**

```yml

```
