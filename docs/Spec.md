# Configuration specification

[[_TOC_]]

## Project

Project name and version.

<table>
<tr>
<th>
Schema
</th>
<th>
Examples
</th>
<th>
Generated
</th>
</tr>
<tr>
<td>

```yml
project:
  - string                      # project name
  - name: string                # project name
    version: !optional scalar   # version
```

</td>
<td>

```yml
project: project_name
```

```yml
project:
  name: project_name
  version: 0.1.0
```

</td>
<td>

```cmake
project(project_name VERSION 0.1.0)
```

</td>
</tr>
</table>

## Options

CMake options.

<table>
<tr>
<th>
Schema
</th>
<th>
Examples
</th>
<th>
Generated
</th>
</tr>
<tr>
<td>

```yml
options: !optional
  $string:                      # option name
    description: string         # description
    default: !optional string   # default value (default: NO)
```

</td>
<td>

```yml
options:
  MY_OPTION:
    description: "My option"
    default: "default value"
```

</td>
<td>

```cmake
option(MY_OPTION "My option" "default value")
```

</td>
</tr>
</table>

## Settings

CMake variables.

<table>
<tr>
<th>
Schema
</th>
<th>
Examples
</th>
<th>
Generated
</th>
</tr>
<tr>
<td>

```yml
settings: !optional map<string;scalar>  # variable name -> value map
```

</td>
<td>

```yml
settings:
  MY_VAR1: "some value"
  MY_VAR2: 42
```

</td>
<td>

```cmake
set(MY_VAR1 "some value")
set(MY_VAR2 42)
```

</td>
</tr>
</table>

## Packages

System and external dependencies.

<table>
<tr>
<th>
Schema
</th>
<th>
Examples
</th>
<th>
Generated
</th>
</tr>
<tr>
<td>

```yml
packages: !optional
  - - external: string                      # unique path to the package
      if: !optional string                  # condition (any CMake expression)
      url: string                           # url to the package repository
      version: !optional scalar             # package version, default: HEAD
      strategy: !optional !variant          # fetch strategy, default: submodule
        - submodule                         # fetch as git submodule
        - clone                             # fetch as raw files
      options: !optional map<string;scalar> # CMake options overrides for the package

    - system: string                # name of the system CMake package
      if: !optional string          # condition
      version: !optional scalar     # package version
      required: !optional boolean   # default: true
```

</td>
<td>

```yml
packages:
  - external: external/doctest
    if: PROJECT_IS_TOP_LEVEL
    url: https://github.com/doctest/doctest
    options:
      DOCTEST_NO_INSTALL: ON

  - system: OpenGL
```

</td>
<td>

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

</td>
</tr>
</table>
