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
 *  \see https://github.com/morinim/ultra/wiki/symbolic_regression
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

int main()
{
  using namespace ultra;

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

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual()
            << "\n\nFITNESS\n" << *result.best_measurements().fitness << '\n';
}
