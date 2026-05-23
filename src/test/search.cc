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

#include "kernel/search.h"
#include "kernel/gp/individual.h"
#include "kernel/de/numerical_refiner.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <atomic>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>

TEST_SUITE("search")
{

TEST_CASE_FIXTURE(fixture1, "ALPS search")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  search s(prob, eva);

  const auto stats(s.run(1));

  CHECK(!stats.best_individual().empty());
  CHECK(eva(stats.best_individual())
        == doctest::Approx(*stats.best_measurements().fitness));
}

TEST_CASE_FIXTURE(fixture1, "refiner is called when appropriate")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  prob.params.refinement.fraction = 1.0;
  prob.params.refinement.stagnation_threshold = 0;
  prob.params.refinement.cooldown = 0;
  prob.params.evolution.generations = 1;

  search s(prob, eva);

  std::atomic_bool called(false);

  auto &ret(
    s.refinement(
      [&](gp::individual &, const auto &,
          const parameters::refinement_parameters &)
      {
        called = true;
        return std::optional<evaluator_fitness_t<decltype(eva)>> {};
      }));

  CHECK(&ret == &s);

  SUBCASE("Less than three layers")
  {
    prob.params.population.init_subgroups = 2;
    s.run();

    CHECK(!called);
  }

  SUBCASE("At least three layers")
  {
    prob.params.population.init_subgroups = 3;
    s.run();

    CHECK(called);
  }
}

TEST_CASE_FIXTURE(fixture1, "DE backend setter compatibility")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  search s(prob, eva);

  CHECK(&s.refinement(de::numerical_refinement_backend()) == &s);
}

TEST_CASE_FIXTURE(fixture1, "refinement backend controls replacement")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::fixed);

  // Use `std_es` so this test observes the refinement contract directly.
  // ALPS may erase or reorganize layers after refinement and before the
  // `after_generation` callback, hiding an accepted replacement.
  using search_t = basic_search<std_es, decltype(eva)>;

  prob.params.population.individuals = 30;
  prob.params.refinement.fraction = 1.0;
  prob.params.refinement.stagnation_threshold = 0;
  prob.params.refinement.cooldown = 0;
  prob.params.evolution.generations = 1;

  // Static storage avoids nested lambda capture issues on MSVC.
  static constexpr individual::age_t marker_age(1000000);

  const auto run_refinement([&](bool accept)
  {
    search_t s(prob, eva);

    std::atomic_bool called(false);
    bool accepted_candidate_found(false);

    s.refinement(
      [&, accept](gp::individual &ind, const auto &,
                  const parameters::refinement_parameters &)
      -> std::optional<double>
      {
        called = true;
        ind.inc_age(marker_age);

        if (accept)
          return -std::numeric_limits<double>::infinity();

        return {};
      });

    s.after_generation(
      [&](const auto &pop, const auto &)
      {
        accepted_candidate_found =
          std::ranges::any_of(pop,
                              [](const auto &ind)
                              {
                                return ind.age() >= marker_age;
                              });
      });

    s.run();

    CHECK(called);
    CHECK(accepted_candidate_found == accept);
  });

  SUBCASE("`nullopt` leaves the population unchanged")
  {
    run_refinement(false);
  }

  SUBCASE("returned fitness accepts the backend result")
  {
    run_refinement(true);
  }
}

TEST_CASE_FIXTURE(fixture1, "refinement is skipped before stagnation threshold")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::fixed);

  prob.params.population.init_subgroups = 3;
  prob.params.evolution.generations = 5;
  prob.params.refinement.fraction = 1.0;
  prob.params.refinement.stagnation_threshold = 10;
  prob.params.refinement.cooldown = 0;

  search s(prob, eva);

  std::atomic_bool called(false);

  s.refinement(
    [&](gp::individual &, const auto &,
        const parameters::refinement_parameters &)
    {
      called = true;
      return std::optional<evaluator_fitness_t<decltype(eva)>> {};
    });

  s.run();

  CHECK(!called);
}

TEST_CASE_FIXTURE(fixture1, "refinement starts after stagnation threshold")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::fixed);
  using search_t = basic_search<std_es, decltype(eva)>;

  prob.params.cache.size = 0;
  prob.params.population.individuals = 30;
  prob.params.evolution.generations = 3;
  prob.params.refinement.fraction = 0.1;
  prob.params.refinement.stagnation_threshold = 2;
  prob.params.refinement.cooldown = 0;

  search_t s(prob, eva);

  std::atomic_bool called(false);
  bool first_refinement_observed(false);
  unsigned first_refinement_generation(0);
  unsigned first_refinement_stagnation(0);

  s.refinement(
    [&](gp::individual &, const auto &,
        const parameters::refinement_parameters &)
    {
      called = true;
      return std::optional<evaluator_fitness_t<decltype(eva)>> {};
    });

  s.after_generation(
    [&](const auto &, const auto &sum)
    {
      if (sum.stagnation() < prob.params.refinement.stagnation_threshold)
        CHECK(!called);

      if (called && !first_refinement_observed)
      {
        first_refinement_observed = true;
        first_refinement_generation = sum.generation;
        first_refinement_stagnation = sum.stagnation();
      }
    });

  s.run();

  CHECK(called);
  REQUIRE(first_refinement_observed);
  CHECK(first_refinement_generation == 2);
  CHECK(first_refinement_stagnation
        == prob.params.refinement.stagnation_threshold);
}

}  // TEST_SUITE
