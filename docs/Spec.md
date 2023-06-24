# Configuration specification

[[_TOC_]]

## Project

Project name and version.

Schema:

```yml
project:
  - string
  - name: string
    version: !optional scalar
```

Examples:

```yml
project: project_name
```

```yml
project:
  name: project_name
  version: 0.1.0
```

Generated configuration:

```cmake
project(project_name VERSION 0.1.0)
```

## Options

CMake options.

Schema:

```yml
options:
  $string:
    description: string
    default: !optional string
```

Examples:

```yml
options:
  MY_OPTION:
    description: "My option"
    default: "default value"
```

Generated configuration:

```cmake
option(MY_OPTION "My option" "default value")
```

