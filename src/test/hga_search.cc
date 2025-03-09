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

TEST_CASE("8 Queens")
{
  using namespace ultra;

  const int NQUEENS(10);

  hga::problem prob;
  prob.insert<hga::permutation>(NQUEENS);

  // Fitness function.
  auto f = [](const hga::individual &x)
  {
    double attacks(0);

    const auto columns(std::get<D_IVECTOR>(x[0]));

    for (int queen(0); queen < NQUEENS - 1; ++queen)  // skips the last queen
    {
      const int row(columns[queen]);

      for (int i(queen + 1); i < NQUEENS; ++i)
      {
        const int other_row(columns[i]);

        if (std::abs(other_row - row) == i - queen)  // diagonal
          ++attacks;
      }
    }

    return -attacks;
  };

  // Let's go.
  hga::search search(prob, f);
  const auto res(search.run(5));

  CHECK(res.best_measurements.fitness == doctest::Approx(0.0));
}

}  // TEST_SUITE("HGA::SEARCH")
