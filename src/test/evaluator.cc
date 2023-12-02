/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cstdlib>
#include <iostream>

#include "kernel/evolution_selection.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVALUATOR")
{

TEST_CASE_FIXTURE(fixture1, "Concepts")
{
  using namespace ultra;

  prob.env.population.individuals = 2;
  prob.env.population.layers      = 1;

  population<gp::individual> pop(prob);

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);
  static_assert(Evaluator<decltype(eva)>);

  auto eva2([&eva](const gp::individual &i) { return eva(i); });
  static_assert(Evaluator<decltype(eva2)>);
}

}  // TEST_SUITE("EVALUATOR")
