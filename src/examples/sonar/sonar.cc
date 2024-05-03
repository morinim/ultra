/*
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
 *  \see
 *  https://github.com/morinim/ultra/wiki/sonar
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

int main()
{
  using namespace ultra;

  // READING INPUT DATA
  src::dataframe::params params;
  params.output_index = src::dataframe::params::index::back;

  src::problem prob("sonar.csv", params);
  prob.params.slp.code_length = 300;

  // SETTING UP SYMBOLS
  prob.setup_symbols();

  // SEARCHING
  src::search s(prob);
  const auto result(s.run());

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual
            << "\n\nFITNESS\n" << *result.best_measurements.fitness << '\n';
}
