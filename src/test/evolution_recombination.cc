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

  test_evaluator<gp::individual> eva(test_evaluator_type::distinct);
  summary<gp::individual, double> sum;
  recombination::base recombine(eva, prob, sum);

  std::vector parents = { gp::individual(prob), gp::individual(prob) };
  while (parents[0] == parents[1])
    parents[1] = gp::individual(prob);

  SUBCASE("No crossover and no mutation")
  {
    prob.env.evolution.p_cross = 0.0;
    prob.env.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(parents).front());

      CHECK(off.is_valid());
      const bool one_or_the_other(off == parents[0] || off == parents[1]);
      CHECK(one_or_the_other);
    }
  }

  SUBCASE("No mutation")
  {
    prob.env.evolution.p_cross = 1.0;
    prob.env.evolution.p_mutation = 0.0;

    std::vector same_parents = {parents[0], parents[0] };

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(same_parents).front());

      CHECK(off.is_valid());
      CHECK(off == same_parents[0]);
    }
  }

  SUBCASE("Standard")
  {
    for (unsigned i(0); i < 100; ++i)
    {
      sum.mutations = 0;
      sum.crossovers = 0;

      const auto off(recombine(parents).front());

      CHECK(off.is_valid());

      const bool distinct(sum.mutations + sum.crossovers == 0
                          || (off != parents[0] && off != parents[1]));
      CHECK(distinct);
    }
  }
}

}  // TEST_SUITE("EVOLUTION RECOMBINATION")
