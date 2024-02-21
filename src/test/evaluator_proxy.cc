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

#include "kernel/evaluator_proxy.h"
#include "kernel/scored_individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVALUATOR_PROXY")
{

TEST_CASE_FIXTURE(fixture1, "Concepts")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva;
  static_assert(Evaluator<decltype(eva)>);

  evaluator_proxy proxy(eva, 7);
  static_assert(Evaluator<decltype(proxy)>);
}

TEST_CASE_FIXTURE(fixture1, "Fitness recall")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  evaluator_proxy proxy(eva, 10);

  std::vector<scored_individual<gp::individual, double>> scored_individuals;
  for (unsigned elems(100); elems; --elems)
  {
    const gp::individual prg(prob);
    scored_individuals.emplace_back(prg, proxy(prg));
  }

  for (const auto &si : scored_individuals)
    CHECK(proxy(si.ind) == doctest::Approx(si.fit));
}

}  // TEST_SUITE("EVALUATOR")
