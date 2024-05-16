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

  // SETTING UP SYMBOLS
  prob.setup_symbols();

  // TWEAKING THE PARAMETERS
  prob.params.evolution.generations = 600;
  prob.params.evolution.brood_recombination = 3;
  prob.params.team.individuals = 3;

  // SEARCHING
  src::search<gp::team<gp::individual>> s(prob);
  //src::search<gp::individual> s(prob);
  s.validation_strategy<src::holdout_validation>(prob);

  const auto result(s.run(5));

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual
            << "\n\nACCURACY\n" << *result.best_measurements.accuracy * 100.0
            << '%'
            << "\n\nFITNESS\n" << *result.best_measurements.fitness << '\n';
}
