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

#include "kernel/evolution.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION")
{

TEST_CASE_FIXTURE(fixture1, "ALPS evolution")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  evolution evo(alps_es(prob, eva));

  const auto sum(evo.run());

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));
}

TEST_CASE_FIXTURE(fixture1, "Shake function")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  evolution evo(std_es(prob, eva));
  evo.shake_function([i = 0](unsigned gen) mutable
                     {
                       CHECK(gen == i);
                       ++i;
                       return true;
                     });

  evo.run();
}

TEST_CASE_FIXTURE(fixture4, "DE evolution")
{
  using namespace ultra;

  prob.params.population.individuals    = 200;
  prob.params.population.init_subgroups =   1;

  test_evaluator<de::individual> eva(test_evaluator_type::realistic);

  evolution evo(de_es(prob, eva));

  const auto sum(evo.run());

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));
}

}  // TEST_SUITE("EVOLUTION")
