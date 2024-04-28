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
 */

#include <cstdlib>
#include <iostream>
#include <numbers>

#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/search.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SRC::SEARCH")
{

TEST_CASE("Symbolic regression")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  // The target function is `x + sin(x)`.
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

  const auto oracle(s.oracle(result.best_individual));
  CHECK(oracle->is_valid());

  const std::vector<std::pair<double, double>> test =
  {
    {-20.9129, -20.0},
    {-15.7121, -16.0},
    {-11.4634, -12.0},
    {  9.4560,  10.0},
    { 11.4634,  12.0}
  };

  for (const auto [out, in] : test)
    CHECK(std::get<D_DOUBLE>((*oracle)({in})) == doctest::Approx(out));
}

}  // TEST_SUITE("SRC::SEARCH")
