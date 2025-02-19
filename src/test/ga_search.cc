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

  const std::string target = "Hello World";
  const std::string CHARSET =
    " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!";

  ga::problem prob(target.length(), {0, CHARSET.size()});

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

  const auto res(search.run(8));

  CHECK(res.best_measurements.fitness == doctest::Approx(target.length()));
}

TEST_CASE("8 Queens")
{
  using namespace ultra;

  const int NQUEENS(8);

  ga::problem prob(NQUEENS, {0, NQUEENS});

  // Fitness function.
  auto f = [](const ga::individual &x)
  {
    double attacks(0);

    for (int queen(0); queen < NQUEENS - 1; ++queen)  // skips the last queen
    {
      const int row(x[queen]);

      for (int i(queen + 1); i < NQUEENS; ++i)
      {
        const int other_row(x[i]);

        if (other_row == row                            // same row
            || std::abs(other_row - row) == i - queen)  // or diagonal
          ++attacks;
      }
    }

    return -attacks;
  };

  // Let's go.
  ga::search search(prob, f);
  auto result = search.run(4);

  CHECK(result.best_measurements.fitness == doctest::Approx(0.0));
}

}  // TEST_SUITE("GA::SEARCH")
