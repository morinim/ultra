[![Ultra](https://github.com/morinim/ultra/wiki/img/logo.png)][homepage]
[![Version](https://img.shields.io/github/tag/morinim/ultra.svg)][releases]

![C++20](https://img.shields.io/badge/c%2B%2B-20-blue.svg)
[![Build and test all platforms](https://github.com/morinim/ultra/actions/workflows/build_release.yml/badge.svg)][build_status]
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8725/badge)][best_practices]
[![License](https://img.shields.io/badge/license-MPLv2-blue.svg)][mpl2]
[![Twitter](https://img.shields.io/twitter/url/https/github.com/morinim/ultra.svg?style=social)][twitter]

## UNDER DEVELOPMENT ##
There is still an ongoing transferring process from Vita and many features aren't available or don't completely work.

This framework will be a major breakthrough but at the moment is far from being production ready ([approximate view of the process](https://github.com/users/morinim/projects/4/)).

Source has been made available for people / companies:
- already using [Vita][vita] to be able to experience the new library;
- who would like to sponsor the project.

## OVERVIEW

Ultra is a scalable, high performance evolutionary algorithms framework.

It's suitable for [classification][classification], [symbolic regression][sr], content base image retrieval, data mining and [software agents][agent] control. Main features:

- concurrency support
- modern, standard ISO C++20 source code
- flexible and fast
- easy integration with other systems
- simple addition of features and modules
- fast experimentation with detailed run-log
- [more][features]

This software is the result of a complete rewriting of [Vita][vita]. The code has been restructured, simplified and if now less *research oriented*; however it maintains all the useful stuffs of the original project and **concurrency is fully supported**. If you're coming from Vita take a look at the [migration notes][migrating].

## EXAMPLES

### Mathematical optimization
<img src="https://github.com/morinim/ultra/wiki/img/rastrigin.png" width="300">

The core of the operation is:

```C++
de::problem prob(dimensions, {-5.12, 5.12});

prob.params.population.individuals =   50;
prob.params.evolution.generations  = 1000;

de::search search(prob, rastrigin_func);

auto res(search.run());

auto solution(res.best_individual);
auto value(res.best_measurements.fitness);
```

Further details in the [specific tutorial][rastrigin].

### Symbolic regression
<img src="https://github.com/morinim/ultra/wiki/img/symbolic_regression02.gif" width="300">

```C++
// DATA SAMPLE (output, input)
// (the target function is `x + sin(x)`)
std::istringstream training(R"(
  -9.456,-10.0
  -8.989, -8.0
  -5.721, -6.0
  -3.243, -4.0
  -2.909, -2.0
   0.000,  0.0
   2.909,  2.0
   3.243,  4.0
   5.721,  6.0
   8.989,  8.0
)");

// READING INPUT DATA
src::problem prob(training);

// SETTING UP SYMBOLS
prob.insert<real::sin>();
prob.insert<real::cos>();
prob.insert<real::add>();
prob.insert<real::sub>();
prob.insert<real::div>();
prob.insert<real::mul>();

// SEARCHING
src::search s(prob);
const auto result(s.run());
```

It's pretty straightforward (further details in the [specific tutorial][sr]).

### Classification
<img src="https://github.com/morinim/ultra/wiki/img/sonar.jpg" width="300">

```C++
  // READING INPUT DATA
  src::dataframe::params params;
  params.output_index = src::dataframe::params::index::back;

  src::problem prob("sonar.csv", params);

  // SETTING UP SYMBOLS
  prob.setup_symbols();

  // SEARCHING
  src::search s(prob);
  s.validation_strategy<src::holdout_validation>(prob);

  const auto result(s.run());
```

Many additional information ins in the [specific tutorial][sonar].

## DOCUMENTATION

There is a [comprehensive wiki][wiki]. You should probably start with the [tutorials][tutorials].

## BUILD REQUIREMENTS

Ultra is designed to have fairly minimal requirements to build and use with your projects, but there are some. Currently, we support Linux and Windows. We will also make our best effort to support other platforms (e.g. Mac OS X, Solaris, AIX).
However, since core members of the Ultra project have no access to these platforms, Ultra may have outstanding issues there. If you notice any problems on your platform, please use the
[issue tracking system][issue]; patches for fixing them are even more welcome!

### Mandatory

* A C++20-standard-compliant compiler
* [CMake][cmake]

### Optional

* [Python v3][python] for additional functionalities

## GETTING THE SOURCE

There are two ways of getting Ultra's source code: you can [download][download] a stable source release in your preferred archive format or directly clone the source from a repository.

Cloning a repository requires a few extra steps and some extra software packages on your system, but lets you track the latest development and make patches much more easily, so we highly encourage it.

Run the following command:

```
git clone https://github.com/morinim/ultra.git
```

## THE ULTRA DISTRIBUTION

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

## SETTING UP THE BUILD

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

## INSTALLING ULTRA

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

## LICENSE

[Mozilla Public License v2.0][mpl2] (also available in the accompanying [LICENSE][license] file).

## VERSIONING
<img align="left" src="https://imgs.xkcd.com/comics/workflow.png">

Ultra uses [semantic versioning][semver]. Releases are tagged.

When a release cannot be backward compatible, because an API is changing, modifications required for upgrading aren't (usually) too difficult.

So don't be afraid of a different *MAJOR* version and read the release notes for details about the breaking changes.

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
[migrating]: https://github.com/morinim/ultra/wiki/migrating
[mpl2]: https://www.mozilla.org/MPL/2.0/
[python]: https://www.python.org/
[rastrigin]: https://github.com/morinim/ultra/wiki/rastrigin_tutorial
[releases]: https://github.com/morinim/ultra/releases
[semver]: https://semver.org/
[sonar]: https://github.com/morinim/ultra/wiki/sonar
[sr]: https://github.com/morinim/ultra/wiki/symbolic_regression
[tutorials]: https://github.com/morinim/ultra/wiki/tutorials
[twitter]: https://twitter.com/intent/tweet?text=%23Ultra+evolutionary+algorithms+framework:&url=https%3A%2F%2Fgithub.com%2Fmorinim%2Fultra
[vita]: https://github.com/morinim/vita
[wiki]: https://github.com/morinim/ultra/wiki
[windows]: https://github.com/morinim/ultra/wiki/win_build
