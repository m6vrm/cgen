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
  - string
  - name: string
    version: !optional scalar
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
options:
  $string:
    description: string
    default: !optional string
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
