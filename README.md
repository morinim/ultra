[![Ultra](https://github.com/morinim/ultra/wiki/img/logo.png)][homepage]
[![Version](https://img.shields.io/github/tag/morinim/ultra.svg)][releases]

![C++20](https://img.shields.io/badge/c%2B%2B-20-blue.svg)
[![Build and test all platforms](https://github.com/morinim/ultra/actions/workflows/build_release.yml/badge.svg)][build_status]
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8725/badge)][best_practices]
[![License](https://img.shields.io/badge/license-MPLv2-blue.svg)][mpl2]
[![Twitter](https://img.shields.io/twitter/url/https/github.com/morinim/ultra.svg?style=social)][twitter]

## This software is under developed ##
There is still an ongoing transferring process from Vita and many features aren't available or don't completely work.

This framework will be a major breakthrough but at the moment is far from being production ready.

It's made available for people / companies:
- already using [Vita][vita] to be able to experience the new library;
- who would like to sponsor the project.

## Overview ##

Ultra is a scalable, high performance evolutionary algorithms framework.

It's suitable for [classification][classification], [symbolic regression][sr], content base image retrieval, data mining and [software agent][agent] implementation. Main features:

- concurrency support
- flexible and fast
- easy integration with other systems
- simple addition of features and modules
- fast experimentation with detailed run-log
- modern, standard ISO C++20 source code
- [more][features]

This software is the result of a complete rewriting of [Vita][vita]. The code has been restructured, simplified and if now less *research oriented*; however it maintains all the useful stuffs of the original project and **concurrency is fully supported**.


## Documentation ##

There is a [comprehensive wiki][wiki]. You should probably start with the [tutorials][tutorials].

## Build requirements ##

Ultra is designed to have fairly minimal requirements to build and use with your projects, but there are some. Currently, we support Linux and Windows. We will also make our best effort to support other platforms (e.g. Mac OS X, Solaris, AIX).
However, since core members of the Ultra project have no access to these platforms, Ultra may have outstanding issues there. If you notice any problems on your platform, please use the
[issue tracking system][issue]; patches for fixing them are even more welcome!

### Mandatory ###

* A C++20-standard-compliant compiler
* [CMake][cmake]

### Optional ###

* [Python v3][python] for additional functionalities

## Getting the source ##

There are two ways of getting Ultra's source code: you can [download][download] a stable source release in your preferred archive format or directly clone the source from a repository.

Cloning a repository requires a few extra steps and some extra software packages on your system, but lets you track the latest development and make patches much more easily, so we highly encourage it.

Run the following command:

```
git clone https://github.com/morinim/ultra.git
```

## The Ultra distribution ##

This is a sketch of the resulting directory structure:
```
ultra/
  doc/
  misc/
  src/
    CMakeLists.txt
    examples/ .............Various examples
    kernel/ ...............Ultra kernel (core library)
    test/ .................Test-suite
    third_party/ ..........Third party libraries
    utility/ ..............Support libraries / files
  tools/ ..................Various tools related to development
  CONTRIBUTING.md
  LICENSE
  README.md
```

## Setting up the build ##

```shell
cd ultra
cmake -B build/ src/
```

To suggest a specific compiler you can write:

```shell
CXX=clang++ cmake -B build/ src/
```

You're now ready to build using the underlying build system tool:

* everything: `cmake --build build/`
* kernel library (`libultra.a`): `cmake --build build/ --target ultra`
* `sr` tool: `cmake --build build/ --target sr`
* tests: `cmake --build build/ --target tests`
* the *ABC* example: `cmake --build build/ --target ABC`
* for a list of valid targets: `cmake --build build/ --target help`

The output files are stored in subdirectories of `build/` (out of source build).

Windows may need various expedients about which you can read in the [Windows walkthrough][windows].

## Installing Ultra ##

To install Ultra use the command:

```shell
cmake --install build/
```
(requires superuser, i.e. root, privileges)

Manually installing is also very easy. There are just two files, both inside the `build/kernel` directory:

- a static library (e.g. `libultra.a` under Unix);
- an automatically generated global/single header (`auto_ultra.h` which can be renamed).

As a side note, the command to build the global header is:

```shell
./tools/single_include.py --src-include-dir src/ --src-include kernel/ultra.h --dst-include mysingleheaderfile.h
```
(must be executed from the repository main directory)

## License ##

[Mozilla Public License v2.0][mpl2] (also available in the accompanying [LICENSE][license] file).

## Versioning ##

Ultra does **not** use semantic versioning. Releases are tagged.

Given a version number `MAJOR.MINOR.PATCH`, increment the:

- `MAJOR` version when there is a new major release or a significant update and/or API stabilization;
- `MINOR` version when minor features are added;
- `PATCH` version when we make tiny changes, likely to go unnoticed by most.

This allows folks, immediately upon hearing about a new release, to get a rough sense of its scope. As to backwards compatibility — ideally every release, even major ones, are backwards-compatible... and when they cannot be, because an API is changing, it should be done in a way that it's not too difficult to upgrade.

Avoiding any change to the API and waiting for a *MAJOR* release to be ready would be a terrific impediment to progress. The alternative of frequently incrementing the *MAJOR* version number is incredibly unhelpful.

---

*USQUE AD FINEM ET **ULTRA***


[agent]: https://github.com/morinim/ultra
[best_practices]: https://www.bestpractices.dev/projects/8725
[build_status]: https://github.com/morinim/ultra/actions/workflows/build_release.yml
[classification]: https://github.com/morinim/ultra
[cmake]: https://cmake.org/
[download]: https://github.com/morinim/ultra/archive/master.zip
[eos]: https://www.eosdev.it/
[features]: https://github.com/morinim/ultra/wiki/features
[homepage]: https://github.com/morinim/ultra
[issue]: https://github.com/morinim/ultra/issues
[license]: https://github.com/morinim/ultra/blob/master/LICENSE
[mpl2]: https://www.mozilla.org/MPL/2.0/
[python]: https://www.python.org/
[releases]: https://github.com/morinim/ultra/releases
[sr]: https://github.com/morinim/ultra
[tutorials]: https://github.com/morinim/ultra/wiki/tutorials
[twitter]: https://twitter.com/intent/tweet?text=%23Ultra+evolutionary+algorithms+framework:&url=https%3A%2F%2Fgithub.com%2Fmorinim%2Fultra
[vita]: https://github.com/morinim/vita
[wiki]: https://github.com/morinim/ultra/wiki
[windows]: https://github.com/morinim/ultra/wiki/win_build
