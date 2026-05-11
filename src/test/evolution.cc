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

#include "kernel/evolution.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <cstdlib>
#include <iostream>

TEST_SUITE("EVOLUTION")
{

TEST_CASE_FIXTURE(fixture1, "ALPS evolution")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  evolution evo(prob, eva);

  const auto sum(evo.run<alps_es>());

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));
}

struct clear_counting_evaluator
{
  using individual_t = ultra::gp::individual;
  using fitness_t = double;

  std::shared_ptr<unsigned> clear_count {std::make_shared<unsigned>(0)};

  [[nodiscard]] double operator()(const individual_t &) const { return 0.0; }

  void clear() const noexcept { ++*clear_count; }
};

TEST_CASE_FIXTURE(fixture1, "Shake function")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  SUBCASE("callback is called with the expected generation number")
  {
    test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

    evolution evo(prob, eva);
    evo.shake_function([i = 0](unsigned gen) mutable
    {
      CHECK(gen == i);
      ++i;
      return evaluation_context::changed;
    });

    evo.run<std_es>();
  }

  SUBCASE("shake invalidates evaluator cache when context changes")
  {
    prob.params.evolution.generations = 3;

    clear_counting_evaluator eva;

    evolution evo(prob, eva);
    evo.shake_function([](unsigned)
    {
      return evaluation_context::changed;
    });

    evo.run<std_es>();

    CHECK(*eva.clear_count >= 3);
  }

  SUBCASE("shake doesn't invalidate evaluator cache when context is unchanged")
  {
    prob.params.evolution.generations = 3;

    clear_counting_evaluator eva;

    evolution evo(prob, eva);
    evo.shake_function([](unsigned)
    {
      return evaluation_context::unchanged;
    });

    evo.run<std_es>();

    CHECK(*eva.clear_count == 0);
  }
}

TEST_CASE_FIXTURE(fixture4, "DE evolution")
{
  using namespace ultra;

  prob.params.population.individuals    = 200;
  prob.params.population.init_subgroups =   1;

  test_evaluator<de::individual> eva(test_evaluator_type::realistic);

  evolution evo(prob, eva);

  const auto sum(evo.run<de_es>());

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));
}

}  // TEST_SUITE
