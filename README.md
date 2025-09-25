# The SC Library

This is the `README` file for `libsc`.

The SC Library provides support for parallel scientific applications.

Copyright (C) 2010 The University of Texas System
Additional copyright (C) 2011 individual authors

`libsc` is written by Carsten Burstedde, Lucas C. Wilcox, Tobin Isaac, and
others.  `libsc` is free software released under the GNU Lesser General
Public Licence version 2.1 (or, at your option, any later version).

Please see doc/release_notes.txt for latest updates.

The official web page for source code and documentation is
[p4est.org](https://www.p4est.org/).
Please send bug reports and ideas for contribution to `p4est@ins.uni-bonn.de`.
You are also welcome to post issues on
[github](https://www.github.com/cburstedde/libsc.git).
Please see the `CONTRIBUTING` file and
our [coding standards](doc/coding_standards.txt) for details.

## Building `libsc`

The build instructions for `p4est` also apply to standalone builds of `libsc`.

### Autotools

The autotools build chain is fully supported.

In a fresh checkout, you may run `./bootstrap` to create the `configure`
script.  Calling `make` will regenerate the tools configuration as needed.
Only in rare cases `./bootstrap` will have to be run again.  The script depends
on existing `autoconf`, `automake`, `libtool` and `pkg-config` tools.

Calling `bootstrap` is *not* required for unpacked `tar` archives, or after
pulling fresh code.

We recommend running `configure` with a relative path from an empty build
directory.  Try

    configure --help

for options and switches.  For development with MPI:

    cd empty/build/directory
    ../relative/path/to/configure --enable-mpi --enable-debug \
        CFLAGS="-O0 -g -Wall -Wextra -Wno-unused-parameter"
    make -j8 V=0

To run tests in parallel, run

    make -j2 check V=0

and to pack a distribution tarball, call `bootstrap` and

    mkdir -p build && cd build && ../configure
    make -j3 distcheck V=0

The `V=0` environment variable significantly unclutters console output.
So far, we have not made `V=0` the default.

### CMake

For faster builds that work on Windows as well as MacOS and Linux, and
that are easily usable from other CMake projects, libsc can be built directly,
or used via FetchContent or ExternalProject from other CMake projects.

MPI and OpenMP are enabled by default, and the default build configuration is RelWithDebInfo:

    cmake -B build
    cmake --build build --parallel

To enable JSON via jansson, first install
[jansson CMake project](https://github.com/akheron/jansson), then specify path where you
installed jansson to CMake, say ~/local:

    cmake -B build -Djansson_ROOT=~/local
    cmake --build build --parallel

To disable MPI:

    cmake -B build -Dmpi=no

To disable OpenMP:

    cmake -B build -Dopenmp=no

To compile with debug options:

    cmake -B build -DCMAKE_BUILD_TYPE=Debug

Optionally, run self-tests:

    ctest --test-dir build

Optionally, install `libsc` like:

    cmake -B build -DCMAKE_INSTALL_PREFIX=~/local
    cmake --install build

The optional examples can be built and tested like:

    cmake -S example -B example/build -DSC_ROOT=~/local
    cmake --build example/build
    ctest --test-dir example/build

#### Distribution packages

For developers, source and binary distribution packages are generated
after building `libsc` by:

    cpack --config build/CPackSourceConfig.cmake
    cpack --config build/CPackConfig.cmake

which creates files:

 * `build/package/SC-<version>-Source.zip` containing source code
 * `build/package/SC-<version>-<platform>.zip` containing binary libraries and
    executables suitable for computers of same operating system and compatible
    CPU arch
