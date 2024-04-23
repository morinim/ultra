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

#include "kernel/evolution_selection.h"
#include "kernel/linear_population.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVALUATOR")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva;
  CHECK(Evaluator<decltype(eva)>);

  auto eva2([&eva](const gp::individual &i) { return eva(i); });
  CHECK(Evaluator<decltype(eva2)>);
}

TEST_CASE_FIXTURE(fixture1, "Test evaluator")
{
  using namespace ultra;

  SUBCASE("Realistic")
  {
    std::vector<gp::individual> distinct;
    for (unsigned i(0); i < 100; ++i)
      if (gp::individual prg(prob);
          std::ranges::find_if(
            distinct,
            [&prg](const auto &rhs)
            { return prg.signature() == rhs.signature(); }) == distinct.end())
        distinct.push_back(prg);

    test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
    std::set<double> fitness;
    for (const auto &prg : distinct)
    {
      const auto f(eva(prg));
      CHECK(!fitness.contains(f));
      fitness.insert(f);
    }
  }

  SUBCASE("Fixed")
  {
    linear_population<gp::individual> p(prob);

    test_evaluator<gp::individual> eva(test_evaluator_type::fixed);
    const auto val(eva(random::element(p)));

    CHECK(std::ranges::all_of(p,
                              [&](const auto &prg)
                              {
                                return eva(prg) == doctest::Approx(val);
                              }));
  }

  SUBCASE("Random")
  {
    const gp::individual prg(prob);

    test_evaluator<gp::individual> eva(test_evaluator_type::random);

    std::vector<int> results(10);
    std::ranges::generate(results,
                          [&prg, &eva] { return static_cast<int>(eva(prg)); });

    const auto [min, max] = std::ranges::minmax(results);

    CHECK(min < max);
  }
}

}  // TEST_SUITE("EVALUATOR")
