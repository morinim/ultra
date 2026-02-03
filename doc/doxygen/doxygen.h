/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *
 *  \mainpage ULTRA
 *
 *  \section Introduction
 *  Welcome to the Ultra project.
 *
 *  This is the reference guide for the APIs. Introductory documentation is
 *  available at the [official site](https://ultraevolution.org).
 *
 *  \section note_sec Notes
 *  New versions of this framework will be available at
 *  https://github.com/morinim/ultra
 *
 *  Please reports any suggestions and/or bugs via the
 *  [issue tracking system](https://github.com/morinim/ultra/issues).
 *
 */

/**
 *  \page page1 Understanding the architecture
 *  - [Anatomy of Ultra](https://github.com/morinim/ultra/wiki/anatomy)
 *  - [Do it yourself](https://github.com/morinim/ultra/wiki/diy)
 *
 *  \page page2 Contributor guidelines
 *  - [Coding style](https://ultraevolution.org/wiki/coding_style/)
 *  - [Development cycle](https://ultraevolution.org/wiki/development_cycle/)
 */

/**
 *  \namespace ultra
 *  The main namespace for the project.
 *
 *  \namespace alps
 *  Contains support functions for the ALPS algorithm.
 *
 *  \namespace ga
 *  Contains classes used for Genetic Algorithms.
 *
 *  \namespace de
 *  Contains classes used for Differential Evolution.
 *
 *  \namespace gp
 *  Contains classes used for Genetic Programming.
 *
 *  \namespace hga
 *  Contains classes used for Heterogeneous Genetic Algorithms.
 *
 *  \namespace src
 *  Contains classes used for symbolic regression and classification tasks.
 *
 *  \namespace out
 *  Contains flags and manipulators to control the output format of individuals.
 *
 *  \namespace internal
 *  The contents of this namespace isn't intended for general consumption: it
 *  contains implementation details, not the interface.
 *  It's recommended giving details their own file and tucking it away in a
 *  detail folder.
 */

/**
 *  \example rastrigin.cc
 *  Mathematical optimization (Differential Evolution).
 *  \example scheduling.cc
 *  Scheduling concurrent jobs for several machines (Differential Evolution).
 *  \example symbolic_regression.cc
 *  \example symbolic_regression01.cc
 *  \example symbolic_regression02.cc
 *  \example symbolic_regression03.cc
 *  Searching the space of mathematical expressions to find the model that
 *  best fits a given dataset (Genetic Programming).
 *  \example sonar.cc
 *  Classification (Genetic Programming).
 */

// This is a global "project documentation" file that supplies the front page
// docs for the project, sets up the groups (for use with /ingroup tags) and
// documents the namespaces all in one place. This allows all the "overviews"
// to be held in one logical place rather than scattered to the winds.
