[![Ultra](https://github.com/morinim/ultra/wiki/img/logo.png)][homepage]
[![Version](https://img.shields.io/github/tag/morinim/ultra.svg)][releases]

![C++20](https://img.shields.io/badge/c%2B%2B-20-blue.svg)
![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey)
[![Build and test all platforms](https://github.com/morinim/ultra/actions/workflows/build_release.yml/badge.svg)][build_status]
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8725/badge)][best_practices]
[![License](https://img.shields.io/badge/license-MPLv2-blue.svg)][mpl2]


## PREMISE

This software is a complete rewrite of [Vita][vita]. The code has been restructured and simplified, making it less *research-oriented*; however it retains all the useful aspects of the original project and **fully supports concurrency**. If you're transitioning from Vita, take a look at the [migration notes][migrating].

We are approaching version 1.0 ([approximate view of the process](https://github.com/users/morinim/projects/4/)).


## OVERVIEW

Ultra is a scalable, high-performance C++ framework for evolutionary algorithms, covering genetic programming, genetic algorithms, and differential evolution.

It is suitable for [classification][classification], [symbolic regression][sr], content-based image retrieval, data mining, and [control of software agents][agent], as well as [mathematical optimisation][rastrigin] and [scheduling][scheduling]. Main features include:

- concurrency support
- modern, standard ISO C++20 source code
- flexibility and speed
- easy integration with other systems
- simple addition of features and modules
- fast experimentation with detailed run-log
- [more][features]


## EXAMPLES

### Mathematical optimisation
<img src="https://github.com/morinim/ultra/wiki/img/rastrigin.png" width="300">

The core operations are:

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
src::raw_data training =  // the target function is `y = x + sin(x)`
{
  {   "Y",  "X"},
  {-9.456,-10.0},
  {-8.989, -8.0},
  {-5.721, -6.0},
  {-3.243, -4.0},
  {-2.909, -2.0},
  { 0.000,  0.0},
  { 2.909,  2.0},
  { 3.243,  4.0},
  { 5.721,  6.0},
  { 8.989,  8.0}
};

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

It is quite straightforward (further details can be found in the [specific tutorial][sr]).

### Classification
<img src="https://github.com/morinim/ultra/wiki/img/sonar.jpg" width="300">

This is a machine learning example that classifies sonar data into two categories: rocks and mines.

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

More information in the [specific tutorial][sonar].


## DOCUMENTATION

Comprehensive documentation is available on the [official site][ultrasite]. We recommended that you start with the [tutorials].
Note that the GitHub wiki, which is the source for the ultraevolution.org site, may contain information on features that are not yet available in official releases.


## BUILD REQUIREMENTS

Ultra is designed to have minimal requirements for building and for use with your projects, though some prerequisites are necessary. Currently, we support Linux and Windows. We will also make our best effort to support other platforms (e.g. macOS, BSD).
However, since core members of the Ultra project have no access to these platforms, Ultra may have outstanding issues there. If you notice any problems on your platform, please use the [issue tracking system][issue]; patches for fixing them are even more welcome!

### Mandatory

* A C++20-standard-compliant compiler
* [CMake][cmake]

### Optional

* [Python 3][python] for additional functionalities


## GETTING THE SOURCE

There are two ways of getting Ultra's source code: you can [download][download] a stable source release in your preferred archive format or directly clone the source from a repository.

Cloning the repository requires a few extra steps and additional software packages on your system, but it enables you to track the latest development and apply patches more easily. We highly encourage cloning for those who wish to contribute.

Run the following command:

```
git clone https://github.com/morinim/ultra.git
```


## THE ULTRA DISTRIBUTION

This is a sketch of the resulting directory structure:
```
ultra/
  data/
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
  SECURITY.md
```


## SETTING UP THE BUILD

```shell
cd ultra
cmake -B build/ src/
```

You're now ready to build using the underlying build system tool:

```shell
cmake --build build/
```

The output files are stored in sub-directories of `build/` (out-of-source build).

Further details are available in the [build walkthrough][build].


## INSTALLING ULTRA

To install Ultra, use the command:

```shell
cmake --install build/
```
(requires superuser, i.e. root, privileges)

Manual installation is also straightforward. There are just two files, both inside the `build/kernel` directory:

- a static library (e.g. `libultra.a` under Unix);
- an automatically generated global/single header (`auto_ultra.h` which can be renamed).

As a side note, to build the global header manually, use:

```shell
./tools/single_include.py --src-include-dir src/ --src-include kernel/ultra.h --dst-include mysingleheaderfile.h
```
(must be executed from the main directory of the repository)


## LICENSE

[Mozilla Public License v2.0][mpl2] (also available in the accompanying [LICENSE][license] file).


## VERSIONING
<img align="left" src="https://imgs.xkcd.com/comics/workflow.png">

Ultra uses [semantic versioning][semver]. Releases are tagged.

When a release is not backward compatible due to API changes, the required modifications for upgrading are usually not too difficult.

So, don't be afraid of a different *MAJOR* version and read the release notes for details about the breaking changes.

---

*USQUE AD FINEM ET **ULTRA***


[agent]: https://github.com/morinim/ultra
[best_practices]: https://www.bestpractices.dev/projects/8725
[build]: https://github.com/morinim/ultra/wiki/build
[build_status]: https://github.com/morinim/ultra/actions/workflows/build_release.yml
[classification]: https://github.com/morinim/ultra/wiki/sonar_tutorial
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
[scheduling]: https://github.com/morinim/ultra/wiki/scheduling_tutorial
[semver]: https://semver.org/
[sonar]: https://github.com/morinim/ultra/wiki/sonar_tutorial
[sr]: https://github.com/morinim/ultra/wiki/symbolic_regression
[tutorials]: https://ultraevolution.org/wiki/tutorials
[vita]: https://github.com/morinim/vita
[ultrasite]: https://ultraevolution.org