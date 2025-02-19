/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cstdlib>
#include <iostream>
#include <numbers>

#include "kernel/hga/individual.h"
#include "kernel/hga/search.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("HGA::SEARCH")
{

TEST_CASE("String guess")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  const std::string target = "Hello World";
  const std::string CHARSET =
    " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!";

  hga::problem prob;

  for (std::size_t i(0); i < target.size(); ++i)
    prob.insert<ultra::hga::integer>(interval<int>(0, CHARSET.size()));

  prob.params.population.individuals = 300;

  hga::search search(prob,
                     [&target, &CHARSET](const ultra::hga::individual &x)
                     {
                       double found(0.0);

                       for (std::size_t i(0); i < target.length(); ++i)
                         if (target[i] == CHARSET[std::get<int>(x[i])])
                           ++found;

                       return found;
                     });

  const auto res(search.run(10));

  CHECK(res.best_measurements.fitness == doctest::Approx(target.length()));
}

}  // TEST_SUITE("HGA::SEARCH")
