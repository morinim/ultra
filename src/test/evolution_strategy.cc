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
#include "kernel/evolution_summary.h"
#include "kernel/distribution.h"
#include "kernel/gp/individual.h"

#include "test/debug_support.h"
#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION STRATEGY")
{

TEST_CASE_FIXTURE(fixture1, "Strategy concept")
{
  using namespace ultra;

  CHECK(Strategy<alps_es<test_evaluator<gp::individual>>>);
  CHECK(Strategy<std_es<test_evaluator<gp::individual>>>);
  CHECK(!Strategy<int>);
}

TEST_CASE_FIXTURE(fixture1, "ALPS strategy")
{
  using namespace ultra;

  for (unsigned ni(2); ni <= 20; ++ni)
    for (unsigned nl(2); nl <= 5; ++nl)
    {
      prob.params.population.individuals = ni;
      prob.params.population.init_layers = nl;

      layered_population<gp::individual> pop(prob);
      const auto range(pop.range_of_layers());

      test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

      const auto initial_best(debug::best_individual(pop, eva));

      summary<gp::individual, double> sum;

      alps_es alps(prob, eva);
      const auto search(
        [&](auto layer_iter)
        {
          auto evolve(alps.operations(pop, layer_iter, sum.starting_status()));

          for (unsigned iterations(prob.params.population.individuals < 50
                                   ? 50 : prob.params.population.individuals);
               iterations; --iterations)
            evolve();
        });

      {
        std::vector<std::jthread> threads;

        for (auto l(range.begin()); l != range.end(); ++l)
          threads.emplace_back(search, l);
      }

      CHECK(std::ranges::all_of(
              pop,
              [](const auto &prg) { return prg.is_valid(); }));

      CHECK(!sum.best().empty());
      CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));

      const auto final_best(debug::best_individual(pop, eva));

      if (eva(final_best) > eva(initial_best))
      {
        CHECK(eva(final_best) == doctest::Approx(sum.best().fit));

        // We must check signature since two individuals may differ just for
        // the introns.
        CHECK(std::ranges::find_if(
                pop,
                [&sum](const auto &prg)
                {
                  return prg.signature() == sum.best().ind.signature();
                }) != pop.end());
      }
      // It may happen that the evolution doesn't find and individual fitter
      // than the best one of the initial population.
      else
      {
        CHECK(eva(final_best) >= sum.best().fit);
      }
    }
}

TEST_CASE_FIXTURE(fixture1, "ALPS increasing fitness")
{
  using namespace ultra;

  prob.params.population.individuals = 100;
  prob.params.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  const auto range(pop.range_of_layers());

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  summary<gp::individual, double> sum;

  alps_es alps(prob, eva);
  const auto search(
    [&](auto layer_iter)
    {
      auto evolve(alps.operations(pop, layer_iter, sum.starting_status()));

      for (auto iterations(prob.params.population.individuals);
           iterations; --iterations)
        evolve();
    });

  std::vector<distribution<double>> previous;

  for (auto repetitions(10); repetitions; --repetitions)
  {
    {
      std::vector<std::jthread> threads;
      threads.reserve(pop.layers());

      for (auto l(range.begin()); l != range.end(); ++l)
        threads.emplace_back(search, l);
    }

    std::vector<distribution<double>> current;
    for (const auto &layer : range)
    {
      distribution<double> dist;

      for (const auto &prg : layer)
        dist.add(eva(prg));

      current.push_back(dist);
    }

    if (!previous.empty())
    {
      for (std::size_t j(0); j < current.size(); ++j)
        CHECK(previous[j].mean() < current[j].mean());
    }

    previous = current;
  }
}

TEST_CASE_FIXTURE(fixture1, "ALPS init")
{
  using namespace ultra;

  prob.params.population.individuals = 100;
  prob.params.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  alps_es alps(prob, eva);
  alps.init(pop);

  for (std::size_t l(0); const auto &layer : pop.range_of_layers())
  {
    if (l < pop.layers() - 1)
      CHECK(layer.max_age() == prob.params.alps.max_age(l));
    else
    {
      using age_t = decltype(pop.back().max_age());
      CHECK(layer.max_age() == std::numeric_limits<age_t>::max());
    }

    ++l;
  }

  CHECK(std::ranges::all_of(pop,
                            [](const auto &prg) { return prg.age() == 0; }));
}

TEST_CASE_FIXTURE(fixture1, "ALPS init / after_generation")
{
  using namespace ultra;

  prob.params.population.individuals = 100;
  prob.params.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  alps_es alps(prob, eva);
  alps.init(pop);

  summary<gp::individual, double> sum;

  SUBCASE("Typical")
  {
    sum.az = analyze(pop, eva);
    alps.after_generation(pop, sum);

    CHECK(std::ranges::all_of(
            pop, [](const auto &prg) { return prg.age() == 1; }));

    CHECK(std::ranges::all_of(pop.range_of_layers(),
                              [](const auto &layer)
                              {
                                return layer.allowed() == layer.size();
                              }));

    CHECK(pop.layers() == prob.params.population.init_layers);
  }

  SUBCASE("Two identical layers")
  {
    pop.front() = pop.layer(1);

    sum.az = analyze(pop, eva);
    CHECK(almost_equal(sum.az.fit_dist(0).mean(), sum.az.fit_dist(1).mean()));

    alps.after_generation(pop, sum);

    CHECK(std::ranges::all_of(pop.range_of_layers(),
                              [](const auto &layer)
                              {
                                return layer.allowed() == layer.size();
                              }));

    CHECK(pop.layers() == prob.params.population.init_layers - 1);
  }

  SUBCASE("Converged layer")
  {
    const gp::individual clone(prob);

    for (auto &prg : pop.layer(1))
      prg = clone;

    sum.az = analyze(pop, eva);
    CHECK(issmall(sum.az.fit_dist(1).standard_deviation()));

    alps.after_generation(pop, sum);

    CHECK(pop.layer(1).allowed() < pop.layer(1).size());
    CHECK(pop.layers() == prob.params.population.init_layers);
  }

  SUBCASE("Age gap")
  {
    auto backup_pop(pop);

    const auto diff(prob.params.alps.max_layers
                    - prob.params.population.init_layers);

    for (unsigned i(1); i <= diff; ++i)
    {
      sum.generation += prob.params.alps.age_gap;
      sum.az = analyze(pop, eva);
      alps.after_generation(pop, sum);

      CHECK(pop.layers() == prob.params.population.init_layers + i);

      for (std::size_t l(0); l < backup_pop.layers(); ++l)
        CHECK(std::ranges::equal(pop.layer(l + i), backup_pop.layer(l)));
    }

    CHECK(pop.layers() == prob.params.alps.max_layers);

    backup_pop = pop;

    sum.generation += prob.params.alps.age_gap;
    sum.az = analyze(pop, eva);
    alps.after_generation(pop, sum);

    CHECK(pop.layers() == prob.params.alps.max_layers);

    CHECK(!std::ranges::equal(pop.front(), backup_pop.front()));
    CHECK(!std::ranges::equal(pop.layer(1), backup_pop.layer(1)));

    for (std::size_t l(2); l < pop.layers(); ++l)
      CHECK(std::ranges::equal(pop.layer(l), backup_pop.layer(l)));
  }
}

TEST_CASE_FIXTURE(fixture1, "Standard strategy")
{
  using namespace ultra;

  prob.params.population.individuals = 200;
  prob.params.population.init_layers =   1;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  summary<gp::individual, double> sum;

  const auto initial_best(debug::best_individual(pop, eva));

  std_es standard(prob, eva);
  auto evolve(standard.operations(pop, pop.range_of_layers().begin(),
                                  sum.starting_status()));

  std::vector<distribution<double>> previous;

  for (auto iters(prob.params.population.individuals); iters; --iters)
    evolve();

  CHECK(std::ranges::all_of(pop,
                            [](const auto &prg) { return prg.is_valid(); }));

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));

  const auto final_best(debug::best_individual(pop, eva));

  if (eva(final_best) > eva(initial_best))
  {
    CHECK(eva(final_best) == doctest::Approx(sum.best().fit));

    // We must check signature since two individuals may differ just for
    // the introns.
    CHECK(std::ranges::find_if(
            pop,
            [&sum](const auto &prg)
            {
              return prg.signature() == sum.best().ind.signature();
            }) != pop.end());
  }
  // It may happen that the evolution doesn't find and individual fitter
  // than the best one of the initial population.
  else
  {
    CHECK(eva(final_best) >= sum.best().fit);
  }
}

}  // TEST_SUITE("EVOLUTION STRATEGY")
