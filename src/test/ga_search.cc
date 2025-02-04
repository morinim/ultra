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

#include "kernel/ga/individual.h"
#include "kernel/ga/search.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("GA::SEARCH")
{

TEST_CASE("Rastrigin")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  const std::string target = "Hello World";
  const std::string CHARSET =
    " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!";

  ga::problem prob(target.length(), interval(0, CHARSET.size()));

  prob.params.population.individuals = 300;

  ga::search search(prob,
                    [&target, &CHARSET](const ultra::ga::individual &x)
                    {
                      double found(0.0);

                      for (std::size_t i(0); i < target.length(); ++i)
                        if (target[i] == CHARSET[x[i]])
                          ++found;

                      return found;
                    });

  const auto res(search.run(10));

  CHECK(res.best_measurements.fitness == doctest::Approx(target.length()));
}

}  // TEST_SUITE("GA::SEARCH")
