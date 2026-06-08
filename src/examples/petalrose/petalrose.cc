/*
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/petalrose_tutorial
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

int main()
{
  using namespace ultra;

  // READING INPUT DATA
  src::problem prob("petalrose.csv");

  // SETTING UP SYMBOLS
  prob.setup_symbols();
  prob.insert<integer::number>(1, 9);
  //prob.params.evolution.brood_recombination = 4;
  prob.params.evolution.generations = 1000;
  //prob.params.population.init_subgroups = 3;

  // SEARCHING
  src::search s(prob);

  prob.params.refinement.de.individuals = 20;
  prob.params.refinement.de.generations = 20;
  //prob.params.refinement.stagnation_threshold = 0;
  //prob.params.refinement.cooldown = 0;
  s.refinement(ultra::de::numerical_refinement_backend());

  const auto result(s.run());

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual()
            << "\n\nFITNESS\n" << *result.best_measurements().fitness << '\n';
}
