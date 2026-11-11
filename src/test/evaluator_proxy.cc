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

struct parallel_evaluator
{
  [[nodiscard]] double operator()(const ultra::gp::individual &) const
  {
    return 0.0;
  }

  bool set_parallel_evaluation(unsigned n, std::size_t min)
  {
    helpers = n;
    min_samples = min;
    return accepted;
  }

  unsigned helpers {0};
  std::size_t min_samples {0};
  bool accepted {true};
};

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

TEST_CASE("Parallel evaluation configuration is forwarded")
{
  ultra::evaluator_proxy proxy(parallel_evaluator {}, 7);

  CHECK(proxy.set_parallel_evaluation(3, 64));
  CHECK(proxy.core().helpers == 3);
  CHECK(proxy.core().min_samples == 64);

  CHECK(proxy.set_parallel_evaluation(0));
  CHECK(proxy.core().helpers == 0);
  CHECK(proxy.core().min_samples == 0);

  ultra::evaluator_proxy rejected(parallel_evaluator {.accepted = false}, 7);
  CHECK_FALSE(rejected.set_parallel_evaluation(2, 32));
  CHECK(rejected.core().helpers == 2);
  CHECK(rejected.core().min_samples == 32);
}

TEST_CASE("Parallel evaluation configuration is optional")
{
  ultra::evaluator_proxy proxy(
    ultra::test_evaluator<ultra::gp::individual> {}, 7);

  CHECK_FALSE(proxy.set_parallel_evaluation(3, 64));
}

}  // TEST_SUITE("EVALUATOR")
