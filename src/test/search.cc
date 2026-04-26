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

#include <cstdlib>
#include <iostream>

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

TEST_CASE_FIXTURE(fixture1, "refiner is called")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  search s(prob, eva);

  bool called(false);

  auto &ret(
    s.refinement(
      [&](gp::individual &, const auto &,
          const parameters::refinement_parameters &)
      {
        called = true;
        return std::optional<evaluator_fitness_t<decltype(eva)>> {};
      }));

  CHECK(&ret == &s);

  prob.params.refinement.fraction = 1.0;
  prob.params.evolution.generations = 1;

  s.run();

  CHECK(called);
}

TEST_CASE_FIXTURE(fixture1, "DE backend setter compatibility")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  search s(prob, eva);

  CHECK(&s.refinement(de::numerical_refinement_backend()) == &s);
}

}  // TEST_SUITE
