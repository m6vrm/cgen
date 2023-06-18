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

## Preprocessing

### Includes

Paths to included and merged configuration files.

Schema:

```yml
includes:
  - [string]
  - - paths: [string]
      parameters: !optional map<string;scalar>
```

Examples:

```yml
includes:
  - path/to/file.yml
  - ...
```

```yml
includes:
  paths:
    - path/to/file.yml
    - ...
  parameters:
    key: value
    ...
```

### Templates

...

