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
 *  \see https://github.com/morinim/ultra/wiki/symbolic_regression01
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

int main()
{
  using namespace ultra;

  // DATA SAMPLE (output, input1, input2)
  // (the target function is `ln(x*x + y*y)`)
  std::istringstream training(R"(
    -2.079, 0.25, 0.25
    -0.693, 0.50, 0.50
     0.693, 1.00, 1.00
     0.000, 0.00, 1.00
     0.000, 1.00, 0.00
     1.609, 1.00, 2.00
     1.609, 2.00, 1.00
     2.079, 2.00, 2.00
  )");

  // READING INPUT DATA
  src::problem prob(training);

  // SETTING UP SYMBOLS
  prob.insert<real::sin>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::mul>();
  prob.insert<real::ln>();

  // SEARCHING
  src::search s(prob);
  const auto result(s.run());

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual
            << "\n\nFITNESS\n" << *result.best_measurements.fitness << '\n';
}
