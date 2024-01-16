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
#include "kernel/layered_population.h"
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

  layered_population<gp::individual> pop(prob);

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);

  gp::individual best(pop.layer(0)[0]), worst(pop.layer(0)[0]);
  double best_fit(eva(best)), worst_fit(eva(worst));
  for (const auto &prg : pop)
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
      replace(pop.layer(0), worst);

    for (const auto &prg : pop.layer(0))
      CHECK(prg == worst);

    CHECK(sum.best.solution == worst);
  }

  SUBCASE("Elitism")
  {
    prob.env.evolution.elitism = 1.0;

    const auto backup(pop);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.layer(0), worst);

    CHECK(std::ranges::equal(pop, backup));
    CHECK(sum.best.solution == worst);

    replace(pop.layer(0), best);
    CHECK(sum.best.solution == best);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.layer(0), best);

    for (const auto &prg : pop.layer(0))
      CHECK(prg == best);
    CHECK(sum.best.solution == best);
  }
}

TEST_CASE_FIXTURE(fixture1, "ALPS")
{
  using namespace ultra;

  prob.env.population.individuals = 25;
  prob.env.population.layers      =  4;

  layered_population<gp::individual> pop(prob);
  alps::set_age(pop);

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);

  gp::individual best(pop.layer(0)[0]), worst(pop.layer(0)[0]);
  double best_fit(eva(best)), worst_fit(eva(worst));
  for (const auto &prg : pop)
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

  replacement::alps replace(eva, prob.env, &sum);

  const unsigned big_age(10000);

  SUBCASE("Best fitness but old")
  {
    // A new best individual is found but it's too old for its layer and all
    // layers are full. Individual shouldn't be lost.
    gp::individual new_best(prob);
    new_best.inc_age(big_age);
    CHECK(eva(new_best) > best_fit);

    for (std::size_t l(0); l < pop.layers() - 1; ++l)
      CHECK(new_best.age() > pop.layer(l).max_age());
    CHECK(new_best.age() <= pop.back().max_age());

    replace({std::ref(pop.front()), std::ref(pop.back())}, new_best);

    CHECK(std::ranges::find(pop.back(), new_best) != pop.back().end());
    CHECK(sum.best.solution == new_best);

    // A new best, very old individual is found and there is free space in an
    // intermediate layer.
    for (std::size_t l(pop.layers() - 1); l; --l)
    {
      pop.layer(l - 1).clear();
      replace({std::ref(pop.layer(l - 1)), std::ref(pop.back())}, new_best);
      CHECK(std::ranges::find(pop.layer(l - 1), new_best)
            != pop.layer(l - 1).end());
    }
  }

  SUBCASE("Random fitness")
  {
    const auto backup(pop);

    for (std::size_t l(0); l < pop.layers() - 1; ++l)
      for (unsigned i(0); i < 10; ++i)
      {
        const auto elem(random::individual(pop.layer(l)));
        replace({std::ref(pop.layer(l)), std::ref(pop.back())}, elem);

        if (const auto it(std::ranges::mismatch(pop.layer(l), backup.layer(l)));
            it.in1 != pop.layer(l).end())
        {
          // `elem` replaces a less fit individual.
          CHECK(*it.in1 == elem);
          CHECK(eva(elem) > eva(*it.in2));

          // only one modified element
          *it.in1 = *it.in2;
          CHECK(std::ranges::equal(pop.layer(l), backup.layer(l)));
        }

        for (std::size_t m(l + 1); m < pop.layers() - 1; ++m)
          CHECK(std::ranges::equal(pop.layer(m), backup.layer(m)));
      }
  }
}

TEST_CASE_FIXTURE(fixture1, "ALPS Concurrency")
{
  using namespace ultra;

  prob.env.population.individuals    = 30;
  prob.env.population.layers         = 10;
  prob.env.evolution.tournament_size = 10;

  layered_population<gp::individual> pop(prob);
  alps::set_age(pop);

  const auto search([&](auto to_layers)
  {
    test_evaluator<gp::individual> eva(test_evaluator_type::fixed);
    summary<gp::individual, double> sum;
    replacement::alps replace(eva, prob.env, &sum);

    for (unsigned i(0); i < 30000; ++i)
    {
      const auto offspring{gp::individual(prob)};
      CHECK(offspring.is_valid());

      replace(to_layers, offspring);
    }
  });

  {
    std::vector<std::jthread> threads;

    const auto range(pop.range_of_layers());
    for (auto l(range.begin()), stop(std::prev(range.end())); l != stop; ++l)
      threads.emplace_back(std::jthread(search,
                                        std::vector{
                                          std::ref(*l),
                                          std::ref(pop.back())}));

    threads.emplace_back(std::jthread(search,
                                      std::vector{std::ref(pop.back())}));
  }

  CHECK(pop.is_valid());
}

TEST_CASE_FIXTURE(fixture1, "Move up layer")
{
  using namespace ultra;

  prob.env.population.individuals    = 30;
  prob.env.population.layers         = 10;

  layered_population<gp::individual> pop(prob);
  alps::set_age(pop);

  test_evaluator<gp::individual> eva(test_evaluator_type::random);
  summary<gp::individual, double> sum;
  replacement::alps replace(eva, prob.env, &sum);

  const auto range(pop.range_of_layers());
  for (auto l(std::prev(range.end())); l != range.begin(); --l)
  {
    const auto backup(*l);

    replace.try_move_up_layer(*std::prev(l), *l);

    std::vector<gp::individual> replaced;
    for (const auto &old : backup)
      if (std::ranges::find(*l, old) == l->end())
        replaced.push_back(old);

    for (const auto &prg : *l)
      if (std::ranges::find(backup, prg) == backup.end())
      {
        CHECK(std::ranges::find(*std::prev(l), prg) != std::prev(l)->end());
        CHECK(std::ranges::any_of(replaced,
                                  [&eva, &prg](const auto &ind)
                                  {
                                    return eva(ind) < eva(prg);
                                  }));
      }
  }
}

}  // TEST_SUITE("EVOLUTION REPLACEMENT")
