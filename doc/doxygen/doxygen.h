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
 *  \mainpage ULTRA v0.0.0
 *
 *  \section Introduction
 *  Welcome to the Ultra project.
 *
 *  This is the reference guide for the APIs. Introductory documentation is
 *  available at the [repository's Wiki](https://github.com/morinim/ultra/wiki).
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
 *  \page page1 ULTRA Architecture
 *  - [Tutorials](https://github.com/morinim/ultra/wiki/tutorials)
 *  - [Anatomy of Vita](https://github.com/morinim/ultra/wiki/anatomy)
 *
 *  \page page2 Contributor guidelines
 *  - [Coding style](https://github.com/morinim/ultra/wiki/coding_style)
 *  - [Development cycle](https://github.com/morinim/ultra/wiki/development_cycle)
 */

/**
 *  \namespace ultra
 *  The main namespace for the project.
 *
 *  \namespace alps
 *  Contains support functions for the ALPS algorithm.
 *
 *  \namespace out
 *  Contains flags and manipulators to control the output format of individuals.
 *
 *  \namespace implementation
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
 */

// This is a global "project documentation" file that supplies the front page
// docs for the project, sets up the groups (for use with /ingroup tags) and
// documents the namespaces all in one place. This allows all the "overviews"
// to be held in one logical place rather than scattered to the winds.
