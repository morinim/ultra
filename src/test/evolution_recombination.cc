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

#include "kernel/evolution_recombination.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION RECOMBINATION")
{

TEST_CASE_FIXTURE(fixture1, "Base")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  recombination::base recombine(eva, prob);

  std::vector parents = { gp::individual(prob), gp::individual(prob) };
  while (parents[0] == parents[1])
    parents[1] = gp::individual(prob);

  SUBCASE("No crossover and no mutation")
  {
    prob.params.evolution.p_cross = 0.0;
    prob.params.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(parents));

      CHECK(off.is_valid());
      const bool one_or_the_other(off == parents[0] || off == parents[1]);
      CHECK(one_or_the_other);
    }
  }

  SUBCASE("No mutation")
  {
    prob.params.evolution.p_cross = 1.0;
    prob.params.evolution.p_mutation = 0.0;

    std::vector same_parents = {parents[0], parents[0] };

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(same_parents));

      CHECK(off.is_valid());
      CHECK(off == same_parents[0]);
    }
  }

  SUBCASE("Standard")
  {
    constexpr unsigned N(200);
    unsigned distinct(0);

    for (unsigned repetitions(N); repetitions; --repetitions)
    {
      const auto off(recombine(parents));
      CHECK(off.is_valid());

      if (off != parents[0] && off != parents[1])
        ++distinct;
    }

    CHECK(static_cast<double>(distinct) / N
          > prob.params.evolution.p_cross - 0.1);
  }
}

TEST_CASE_FIXTURE(fixture4, "DE")
{
  using namespace ultra;

  recombination::de recombine(prob);

  prob.params.evolution.p_cross = 0;
  SUBCASE("zero p_cross")
  {
    prob.params.evolution.p_cross = 0;

    for (unsigned iterations(100); iterations; --iterations)
    {
      const de::individual p(prob);
      de::individual a(prob), b(prob), c(prob);

      const auto x(p.crossover(prob.params.evolution.p_cross,
                               prob.params.de.weight,
                               a, b, c));

      for (std::size_t i(0); i < x.parameters() - 1; ++i)
        CHECK(p[i] == doctest::Approx(x[i]));

      CHECK(p[p.parameters() - 1] != doctest::Approx(x[x.parameters() - 1]));
    }
  }
}

}  // TEST_SUITE("EVOLUTION RECOMBINATION")
