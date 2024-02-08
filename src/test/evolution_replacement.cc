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

#include <ranges>

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

  const auto [worst_it, best_it] =
    std::ranges::minmax_element(pop,
                                [&eva](const auto &i1, const auto &i2)
                                {
                                  return eva(i1) < eva(i2);
                                });

  const scored_individual worst(*worst_it, eva(*worst_it));
  const scored_individual best(*best_it, eva(*best_it));

  evolution_status<gp::individual, double> status;
  replacement::tournament replace(eva, prob.env);

  SUBCASE("No elitism")
  {
    prob.env.evolution.elitism = 0.0;

    // This is very important: a value greater than `1` will make the selection
    // of the best element extremely hard.
    prob.env.evolution.tournament_size = 1;

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.front(), worst.ind, status);

    for (const auto &prg : pop.front())
      CHECK(prg == worst.ind);

    CHECK(status.best().ind == worst.ind);
  }

  SUBCASE("Elitism")
  {
    prob.env.evolution.elitism = 1.0;

    const auto backup(pop);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.front(), worst.ind, status);

    CHECK(std::ranges::equal(pop, backup));
    CHECK(status.best().ind == worst.ind);

    replace(pop.front(), best.ind, status);
    CHECK(status.best().ind == best.ind);

    for (unsigned i(0); i < prob.env.population.individuals * 100; ++i)
      replace(pop.front(), best.ind, status);

    for (const auto &prg : pop.front())
      CHECK(prg == best.ind);
    CHECK(status.best().ind == best.ind);
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

  const auto [worst_it, best_it] =
    std::ranges::minmax_element(pop,
                                [&eva](const auto &i1, const auto &i2)
                                {
                                  return eva(i1) < eva(i2);
                                });

  const scored_individual worst(*worst_it, eva(*worst_it));
  const scored_individual best(*best_it, eva(*best_it));

  evolution_status<gp::individual, double> status;
  replacement::alps replace(eva, prob.env);

  const unsigned big_age(10000);

  SUBCASE("Best fitness but old")
  {
    // A new best individual is found but it's too old for its layer and all
    // layers are full. Individual shouldn't be lost.
    gp::individual new_best(prob);
    new_best.inc_age(big_age);
    CHECK(eva(new_best) > best.fit);

    for (const auto &layer : pop.range_of_layers())
      if (&layer == &pop.back())
        CHECK(new_best.age() <= layer.max_age());
      else
        CHECK(new_best.age() > layer.max_age());

    replace(std::vector{std::ref(pop.front()), std::ref(pop.back())},
            new_best, status);

    CHECK(std::ranges::find(pop.front(), new_best) == pop.front().end());
    CHECK(std::ranges::find(pop.back(), new_best) != pop.back().end());
    CHECK(status.best().ind == new_best);

    // A new best, very old individual is found and there is free space in an
    // intermediate layer.
    for (auto &layer : pop.range_of_layers())
    {
      layer.clear();
      replace(std::vector{std::ref(layer), std::ref(pop.back())},
              new_best, status);
      CHECK(std::ranges::find(layer, new_best) != layer.end());
    }
  }

  SUBCASE("Random fitness")
  {
    const auto backup(pop);
    auto b(backup.range_of_layers().begin());

    const auto rng(pop.range_of_layers());

    for (auto l(rng.begin()), stop(std::prev(rng.end())); l != stop; ++l, ++b)
      for (unsigned i(0); i < 10; ++i)
      {
        const auto elem(random::individual(*l));

        replace(alps::replacement_layers(pop, l), elem, status);

        if (const auto it(std::ranges::mismatch(*l, *b));
            it.in1 != l->end())
        {
          // `elem` replaces a less fit individual.
          CHECK(*it.in1 == elem);
          CHECK(eva(elem) > eva(*it.in2));

          // only one modified element
          *it.in1 = *it.in2;
          CHECK(std::ranges::equal(*l, *b));
        }

        auto upper_b(std::next(b));
        for (auto upper_l(std::next(l)); upper_l != stop; ++upper_l, ++upper_b)
          CHECK(std::ranges::equal(*upper_l, *upper_b));
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

  test_evaluator<gp::individual> eva(test_evaluator_type::fixed);
  replacement::alps replace(eva, prob.env);

  const auto search([&](auto to_layers)
  {
    evolution_status<gp::individual, double> status;

    for (unsigned i(0); i < 30000; ++i)
    {
      const auto offspring{gp::individual(prob)};
      CHECK(offspring.is_valid());

      replace(to_layers, offspring, status);
    }
  });

  {
    std::vector<std::jthread> threads;

    const auto range(pop.range_of_layers());
    for (auto l(range.begin()); l != range.end(); ++l)
      threads.emplace_back(search, alps::replacement_layers(pop, l));
  }

  CHECK(pop.is_valid());
}

TEST_CASE_FIXTURE(fixture1, "Move up layer")
{
  using namespace ultra;

  prob.env.population.individuals = 30;
  prob.env.population.layers      = 10;

  layered_population<gp::individual> pop(prob);
  alps::set_age(pop);

  test_evaluator<gp::individual> eva(test_evaluator_type::random);
  evolution_status<gp::individual, double> status;
  replacement::alps replace(eva, prob.env);

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
