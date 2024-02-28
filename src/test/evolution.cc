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
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION")
{

TEST_CASE_FIXTURE(fixture1, "ALPS evolution")
{
  using namespace ultra;

  prob.env.population.individuals = 30;
  prob.env.population.init_layers =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  evolution evo(alps_es(prob, eva));

  const auto sum(evo.run());

  CHECK(!sum.best().empty());
  CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));
}

}  // TEST_SUITE("EVOLUTION")
