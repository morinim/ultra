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

#include "kernel/de/individual.h"
#include "kernel/de/search.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("DE::SEARCH")
{

TEST_CASE("Rastrigin")
{
  using namespace ultra;

  const unsigned dimensions(5);  // 5D - Rastrigin function

  de::problem prob(dimensions, {-5.12, 5.12});

  prob.params.population.individuals =   50;
  prob.params.evolution.generations  = 1000;

  de::search search(prob,
                    [](const ultra::de::individual &x)
                    {
                      using std::numbers::pi;
                      constexpr double A = 10.0;

                      const double rastrigin =
                        A * x.parameters()
                        + std::accumulate(x.begin(), x.end(), 0.0,
                                          [=](double sum, double xi)
                                          {
                                            return sum + xi*xi
                                                   - A*std::cos(2*pi*xi);
                                          });

                      return -rastrigin;
                    });

  const auto res(search.run());

  CHECK(res.best_measurements.fitness == doctest::Approx(0.0));
}

}  // TEST_SUITE("DE::SEARCH")
