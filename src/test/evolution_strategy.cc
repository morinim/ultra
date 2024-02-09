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
      prob.env.population.individuals = ni;
      prob.env.population.init_layers = nl;

      layered_population<gp::individual> pop(prob);
      const auto range(pop.range_of_layers());

      test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

      summary<gp::individual, double> sum;

      alps_es alps(pop, eva, sum.starting_status());
      const auto search(
        [&](auto layer_iter)
        {
          auto evolve(alps.operations(layer_iter));

          for (unsigned iterations(prob.env.population.individuals < 50
                                   ? 50 : prob.env.population.individuals);
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

      const auto best(std::ranges::max(pop,
                                       [&eva](const auto &p1, const auto &p2)
                                       {
                                         return eva(p1) < eva(p2);
                                       }));

      //CHECK(eva(best) <= sum.best().fit);
      //CHECK(std::ranges::find(pop, sum.best().ind) != pop.end());
    }
}

TEST_CASE_FIXTURE(fixture1, "ALPS increasing fitness")
{
  using namespace ultra;

  prob.env.population.individuals = 100;
  prob.env.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  const auto range(pop.range_of_layers());

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  summary<gp::individual, double> sum;

  alps_es alps(pop, eva, sum.starting_status());
  const auto search(
    [&](auto layer_iter)
    {
      auto evolve(alps.operations(layer_iter));

      for (auto iterations(prob.env.population.individuals);
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

  prob.env.population.individuals = 100;
  prob.env.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  summary<gp::individual, double> sum;

  alps_es alps(pop, eva, sum.starting_status());
  alps.init();

  for (std::size_t l(0); const auto &layer : pop.range_of_layers())
  {
    if (l < pop.layers() - 1)
      CHECK(layer.max_age() == prob.env.alps.max_age(l));
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

  prob.env.population.individuals = 100;
  prob.env.population.init_layers =   5;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  summary<gp::individual, double> sum;

  alps_es alps(pop, eva, sum.starting_status());
  alps.init();

  analyzer<gp::individual, double> az;

  const auto analyze_pop([&az, &pop, &eva]
  {
    for (auto it(pop.begin()), end(pop.end()); it != end; ++it)
      az.add(*it, eva(*it), it.layer());
  });

  SUBCASE("Typical")
  {
    analyze_pop();
    alps.after_generation(az);

    CHECK(std::ranges::all_of(
            pop, [](const auto &prg) { return prg.age() == 1; }));

    CHECK(std::ranges::all_of(pop.range_of_layers(),
                              [](const auto &layer)
                              {
                                return layer.allowed() == layer.size();
                              }));

    CHECK(pop.layers() == prob.env.population.init_layers);
  }

  SUBCASE("Two identical layers")
  {
    pop.front() = pop.layer(1);

    analyze_pop();
    CHECK(almost_equal(az.fit_dist(0).mean(), az.fit_dist(1).mean()));

    alps.after_generation(az);

    CHECK(std::ranges::all_of(pop.range_of_layers(),
                              [](const auto &layer)
                              {
                                return layer.allowed() == layer.size();
                              }));

    CHECK(pop.layers() == prob.env.population.init_layers - 1);
  }

  SUBCASE("Converged layer")
  {
    const gp::individual clone(prob);

    for (auto &prg : pop.layer(1))
      prg = clone;

    analyze_pop();
    CHECK(issmall(az.fit_dist(1).standard_deviation()));

    alps.after_generation(az);

    CHECK(pop.layer(1).allowed() < pop.layer(1).size());
    CHECK(pop.layers() == prob.env.population.init_layers);
  }

  SUBCASE("Age gap")
  {
    auto backup_pop(pop);

    const auto diff(prob.env.alps.max_layers
                    - prob.env.population.init_layers);

    for (unsigned i(1); i <= diff; ++i)
    {
      sum.generation += prob.env.alps.age_gap;

      analyze_pop();
      alps.after_generation(az);

      CHECK(pop.layers() == prob.env.population.init_layers + i);

      for (std::size_t l(0); l < backup_pop.layers(); ++l)
        CHECK(std::ranges::equal(pop.layer(l + i), backup_pop.layer(l)));
    }

    CHECK(pop.layers() == prob.env.alps.max_layers);

    backup_pop = pop;

    sum.generation += prob.env.alps.age_gap;

    analyze_pop();
    alps.after_generation(az);

    CHECK(pop.layers() == prob.env.alps.max_layers);

    CHECK(!std::ranges::equal(pop.front(), backup_pop.front()));
    CHECK(!std::ranges::equal(pop.layer(1), backup_pop.layer(1)));

    for (std::size_t l(2); l < pop.layers(); ++l)
      CHECK(std::ranges::equal(pop.layer(l), backup_pop.layer(l)));
  }
}

TEST_CASE_FIXTURE(fixture1, "Standard strategy")
{
  using namespace ultra;

  prob.env.population.individuals = 200;
  prob.env.population.init_layers =   1;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  summary<gp::individual, double> sum;

  std_es standard(pop, eva, sum.starting_status());
  auto evolve(standard.operations(pop.range_of_layers().begin()));

  std::vector<distribution<double>> previous;

  for (auto iteration(prob.env.population.individuals); iteration; --iteration)
    evolve();

  CHECK(std::ranges::all_of(pop,
                            [](const auto &prg) { return prg.is_valid(); }));

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));

  const auto best(std::ranges::max(pop,
                                   [&eva](const auto &p1, const auto &p2)
                                   {
                                     return eva(p1) < eva(p2);
                                   }));

  CHECK(best == sum.best().ind);
  CHECK(eva(best) <= sum.best().fit);
  CHECK(std::ranges::find(pop, sum.best().ind) != pop.end());
}

}  // TEST_SUITE("EVOLUTION STRATEGY")
