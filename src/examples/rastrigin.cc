/*
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/vita/wiki/rastrigin_tutorial
 */

#include <numbers>
#include "kernel/ultra.h"

double neg_rastrigin(const ultra::de::individual &x)
{
  using std::numbers::pi;
  constexpr double A = 10.0;

  const double rastrigin =
    A * x.parameters()
    + std::accumulate(x.begin(), x.end(), 0.0,
                      [=](double sum, double xi)
                      {
                        return sum + xi*xi - A*std::cos(2*pi*xi);
                      });

  return -rastrigin;
}

int main()
{
  using namespace ultra;

  log::reporting_level = log::lINFO;

  const unsigned dimensions(5);  // 5D - Rastrigin function

  de::problem prob(dimensions, {-5.12, 5.12});

  prob.params.population.individuals =   50;
  prob.params.evolution.generations  = 1000;

  de::search search(prob, neg_rastrigin);

  const auto res(search.run());

  const auto solution(res.best_individual);
  const auto value(res.best_measurements.fitness);

  std::cout << "Minimum found: " <<  *value << '\n';

  std::cout << "Coordinates: [ ";
  for (auto xi : solution)
    std::cout << xi << ' ';
  std::cout << "]\n";
}
