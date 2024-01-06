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

#include "kernel/evolution_replacement.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION REPLACEMENT")
{

TEST_CASE_FIXTURE(fixture1, "Tournament")
{
  using namespace ultra;

  prob.env.population.individuals = 20;
  prob.env.population.layers      =  1;

  population<gp::individual> pop(prob);

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);

  gp::individual best(pop.layer(0)[0]), worst(pop.layer(0)[0]);
  double best_fit(eva(best)), worst_fit(eva(worst));
  for (const auto & prg : pop.layer(0))
    if (double curr_fit(eva(prg)); curr_fit < worst_fit)
    {
      worst = prg;
      worst_fit = curr_fit;
    }
    else if (curr_fit > best_fit)
    {
      best = prg;
      best_fit = curr_fit;
    }

  summary<gp::individual, double> sum;
  sum.best.solution = worst;
  sum.best.score.fitness = worst_fit;

  replacement::tournament replace(eva, prob.env, &sum);

  SUBCASE("No elitism")
  {
    prob.env.evolution.elitism = 0.0;

    // This is very important: a value greater than `1` will make the selection
    // of the best element extremely hard.
    prob.env.evolution.tournament_size = 1;

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.layer(0), std::vector{worst});

    for (const auto &prg : pop.layer(0))
      CHECK(prg == worst);

    CHECK(sum.best.solution == worst);
  }

  SUBCASE("Elitism")
  {
    prob.env.evolution.elitism = 1.0;

    const auto backup(pop);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.layer(0), std::vector{worst});

    CHECK(std::ranges::equal(pop, backup));
    CHECK(sum.best.solution == worst);

    replace(pop.layer(0), std::vector{best});
    CHECK(sum.best.solution == best);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.layer(0), std::vector{best});

    for (const auto &prg : pop.layer(0))
      CHECK(prg == best);
    CHECK(sum.best.solution == best);
  }
}

}  // TEST_SUITE("EVOLUTION REPLACEMENT")
