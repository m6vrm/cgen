# cgen

Declarative CMake configuration generator.

Features:

- Declarative CMake configuration with YAML
- Dependency resolution and version pinning
- Different target types support
- Configuration reuse with includes and templates

> :warning: A very limited subset of CMake features is supported.

## Building and installation

Requirements:

- CMake
- GCC/Clang with C++20 standard support

```sh
$ git clone --recurse-submodule https://gitlab.com/madyanov/cgen
$ cd cgen
$ make
$ make install
```

## Usage

- Create `.cgen.yml` configuration file in the root of the project
    - [Configuration specification](docs/Spec.md)
- Generate `CMakeLists.txt` with `cgen -g`
- See help with `cgen -h`

## Configuration specification

See [docs/Spec.md](docs/Spec.md).

## Contributing

- Use [cgen](https://gitlab.com/madyanov/cgen) itself to generate the `CMakeLists.txt` file
- Run all CI checks locally with `make ci`
- List available Make targets with `make help`
- Be sure not to use almost 20-years-old default Make 3.81 on macOS
