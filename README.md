Features
========

*   Declarative CMake configuration with YAML
*   Dependency resolution and version pinning
*   Different target types support
*   Configuration reuse with includes and templates

NOTE: A very limited subset of CMake features is supported.

Installation
============

Requirements:

*   CMake 3.11
*   C++20 compatible compiler

<!-- -->

    $ git clone --recurse-submodules https://m6v.ru/git/cgen
    $ cd cgen
    $ make
    $ sudo make install

Usage
=====

Create .cgen.yml configuration file in the root of the project and generate
CMakeLists.txt with following command:

    $ cgen -g

Specification
=============

See docs/spec.md.

Contribution
============

Use cgen itself to generate the CMakeLists.txt file.

    $ make format       # format source code
    $ make clean test   # run tests
    $ make check        # run static checks
    $ make clean asan   # run tests with address sanitizer
    $ make clean ubsan  # run tests with UB sanitizer
