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

#include "kernel/evolution_strategy.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION STRATEGY")
{

TEST_CASE_FIXTURE(fixture1, "ALPS strategy")
{
  using namespace ultra;

  for (unsigned ni(2); ni <= 20; ++ni)
    for (unsigned nl(2); nl <= 5; ++nl)
    {
      prob.env.population.individuals = ni;
      prob.env.population.layers      = nl;

      layered_population<gp::individual> pop(prob);
      test_evaluator<gp::individual> eva(test_evaluator_type::distinct);

      evolution_status<gp::individual, double> status;

      const auto search(
        [&](auto layer_iter)
        {
          alps_es es(pop, layer_iter, eva, status);

          const unsigned sup(prob.env.population.individuals < 50
                             ? 50 : prob.env.population.individuals);
          for (unsigned k(0); k < sup; ++k)
            es();
        });

      {
        std::vector<std::jthread> threads;

        const auto range(pop.range_of_layers());
        for (auto l(range.begin()); l != range.end(); ++l)
          threads.emplace_back(std::jthread(search, l));
      }

      CHECK(std::ranges::all_of(
              pop,
              [](const auto &prg) { return prg.is_valid(); }));

      CHECK(!status.best().empty());
      CHECK(eva(status.best().ind)
            == doctest::Approx(status.best().fit));

      const auto best(std::ranges::max(pop,
                                       [&eva](const auto &p1, const auto &p2)
                                       {
                                         return eva(p1) < eva(p2);
                                       }));

      CHECK(eva(best) <= status.best().fit);
      //CHECK(std::ranges::find(pop, status.best().ind) != pop.end());
    }
}

}  // TEST_SUITE("EVOLUTION STRATEGY")
