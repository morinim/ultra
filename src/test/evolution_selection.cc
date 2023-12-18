/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cstdlib>
#include <iostream>

#include "kernel/evolution_selection.h"
#include "kernel/distribution.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION SELECTION")
{

TEST_CASE_FIXTURE(fixture1, "Tournament")
{
  using namespace ultra;

  prob.env.population.individuals = 20;
  prob.env.population.layers      =  1;

  // The test assumes independent draws.
  prob.env.evolution.mate_zone = std::numeric_limits<std::size_t>::max();

  // Individuals have distinct ages.
  const auto pop(make_debug_population<gp::individual>(prob));

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);

  selection::tournament select(eva, prob.env);

  // Every individual has a unique fitness (`test_evaluator_type::distinct`),
  // so there is just one maximum-fitness-individual.
  for (unsigned ts(1); ts < prob.env.population.individuals; ++ts)
  {
    prob.env.evolution.tournament_size = ts;

    auto max(std::ranges::max(pop, [eva](const auto &p1, const auto &p2)
                                   {
                                     return eva(p1) < eva(p2);
                                   }));

    const unsigned n(prob.env.population.individuals * 100);
    unsigned found(0);
    for (unsigned i(0); i < n; ++i)
    {
      auto parents(select(pop));

      CHECK(parents.size() == prob.env.evolution.tournament_size);

      const bool is_sorted(
        std::ranges::is_sorted(parents,
                               [&](const auto &p1, const auto &p2)
                               {
                                 return eva(p1) > eva(p2);
                               }));
      CHECK(is_sorted);

      if (std::ranges::find_if(parents,
                               [ma = max.age()](const auto &prg)
                               {
                                 return prg.age() == ma;
                               })
          != parents.end())
        ++found;
    }

    const double frequency(static_cast<double>(found) / n);
    const double p_not_present((pop.size() - 1)
                               / static_cast<double>(pop.size()));
    const double p_present(1.0 - std::pow(p_not_present, ts));
    CHECK(frequency > p_present - 0.1);
    CHECK(frequency < p_present + 0.1);
  }
}

TEST_CASE_FIXTURE(fixture4, "DE")
{
  using namespace ultra;

  prob.env.population.individuals = 100;
  prob.env.population.layers      =   1;

  // The test assumes independent draws.
  prob.env.evolution.mate_zone = std::numeric_limits<std::size_t>::max();

  distribution<double> dist;

  // Individuals have distinct ages.
  const auto pop(make_debug_population<de::individual>(prob));

  test_evaluator<de::individual> eva(test_evaluator_type::distinct);

  selection::de select(eva, prob.env);

  auto max(std::ranges::max(pop, [eva](const auto &p1, const auto &p2)
                                 {
                                   return eva(p1) < eva(p2);
                                 }));

  const unsigned n(prob.env.population.individuals * 100);
  unsigned found(0);
  for (unsigned i(0); i < n; ++i)
  {
    auto parents(select(pop));
    CHECK(parents.size() == 4);

    if (std::ranges::find_if(parents,
                             [ma = max.age()](const auto &prg)
                             {
                               return prg.age() == ma;
                             })
        != parents.end())
      ++found;

    for (const auto &prg : parents)
      dist.add(prg.age());
  }

  const double frequency(static_cast<double>(found) / n);
  const double p_not_present((pop.size() - 1)
                             / static_cast<double>(pop.size()));
  const double p_present(1.0 - std::pow(p_not_present, 4));
  CHECK(frequency > p_present - 0.1);
  CHECK(frequency < p_present + 0.1);

  const double avg(pop.size() / 2);
  const double delta(pop.size() / 20);
  CHECK(avg - delta <= dist.mean());
  CHECK(dist.mean() <= avg + delta);
}

}  // TEST_SUITE("EVOLUTION SELECTION")
