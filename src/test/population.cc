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
#include <map>
#include <sstream>

#include "kernel/population.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("POPULATION")
{

TEST_CASE_FIXTURE(fixture1, "Creation")
{
  using namespace ultra;

  prob.env.population.layers = 1;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(30, 200);

    population<gp::individual> pop(prob);

    CHECK(pop.individuals() == prob.env.population.individuals);
    CHECK(pop.is_valid());
  }
}

TEST_CASE_FIXTURE(fixture1, "Layers and individuals")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(30u, 200u);
    prob.env.population.layers = random::between(1u, 10u);

    population<gp::individual> pop(prob);

    for (std::size_t l(0); l < pop.layers(); ++l)
    {
      const auto n(random::sup(pop.individuals(l)));

      const auto before(pop.individuals(l));

      for (unsigned j(0); j < n; ++j)
        pop.pop_from_layer(l);

      CHECK(pop.individuals(l) == before - n);
    }

    std::size_t count(std::accumulate(pop.begin(), pop.end(), 0u,
                                      [](auto acc, auto) { return ++acc; }));

    CHECK(count == pop.individuals());

    const unsigned added_layers(10);
    for (unsigned j(0); j < added_layers; ++j)
    {
      pop.add_layer();
      CHECK(pop.layers() == prob.env.population.layers + j + 1);
    }

    for (unsigned j(0); j < added_layers; ++j)
    {
      pop.remove_layer(random::sup(pop.layers()));
      CHECK(pop.layers() == prob.env.population.layers + added_layers - j - 1);
    }
  }
}
/*
TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace vita;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.individuals = random::between(30, 300);

    std::stringstream ss;
    population<i_mep> pop1(prob);

    CHECK(pop1.save(ss));

    decltype(pop1) pop2(prob);
    CHECK(pop2.load(ss, prob));
    CHECK(pop2.is_valid());

    CHECK(pop1.layers() == pop2.layers());
    CHECK(pop1.individuals() == pop2.individuals());
    for (unsigned l(0); l < pop1.layers(); ++l)
    {
      CHECK(pop1.individuals(l) == pop2.individuals(l));

      for (unsigned j(0); j < pop1.individuals(); ++j)
      {
        const population<i_mep>::coord c{l, j};
        CHECK(pop1[c] == pop2[c]);
      }
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Pickup")
{
  prob.env.individuals = 30;
  prob.env.layers = 1;

  vita::population<vita::i_mep> pop(prob);

  for (unsigned i(0); i < 10; ++i)
  {
    std::map<vita::population<vita::i_mep>::coord, int> frequency;

    const int draws(5000 * pop.individuals());
    for (int j(0); j < draws; ++j)
      ++frequency[vita::pickup(pop)];

    const int expected(draws / pop.individuals());
    const int tolerance(expected / 10);

    for (const auto &p : frequency)
      CHECK(std::abs(p.second - expected) <= tolerance);

    pop.add_layer();
  }
}
*/
}  // TEST_SUITE("POPULATION")
