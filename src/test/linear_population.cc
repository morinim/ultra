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
#include <map>
#include <sstream>

#include "kernel/linear_population.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("LINEAR POPULATION")
{

TEST_CASE_FIXTURE(fixture1, "Creation")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(1, 100);

    linear_population<gp::individual> pop(prob);

    CHECK(pop.size() == prob.env.population.individuals);
    CHECK(pop.is_valid());
  }
}

TEST_CASE_FIXTURE(fixture1, "Age")
{
  using namespace ultra;

  prob.env.population.individuals = 10;

  linear_population<gp::individual> pop(prob);

  CHECK(std::ranges::all_of(pop, [](const auto &i) { return i.age() == 0; }));

  pop.inc_age();

  CHECK(std::ranges::all_of(pop, [](const auto &i) { return i.age() == 1; }));
}

TEST_CASE_FIXTURE(fixture1, "Iterators")
{
  using namespace ultra;

  for (unsigned i(0); i < 10; ++i)
  {
    prob.env.population.individuals = random::between(30, 200);

    linear_population<gp::individual> pop(prob);

    CHECK(std::distance(pop.begin(), pop.end()) == pop.size());
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(10, 50);

    linear_population<gp::individual> pop1(prob);
    pop1.max_age(1234);

    std::stringstream ss;
    CHECK(pop1.save(ss));

    decltype(pop1) pop2(prob);
    CHECK(pop2.load(ss, prob.sset));
    CHECK(pop2.is_valid());

    CHECK(pop1.size() == pop2.size());
    CHECK(std::ranges::equal(pop1, pop2));
    CHECK(pop1.max_age() == pop2.max_age());
  }
}

TEST_CASE_FIXTURE(fixture1, "Coord")
{
  using namespace ultra;

  prob.env.population.individuals = 30;

  linear_population<gp::individual> pop(prob);

  for (unsigned i(0); i < 10; ++i)
  {
    std::map<linear_population<gp::individual>::coord, int> frequency;

    const int draws(5000 * pop.size());
    for (int j(0); j < draws; ++j)
      ++frequency[random::coord(pop)];

    const int expected(draws / pop.size());
    const int tolerance(expected / 10);

    for (const auto &p : frequency)
      CHECK(std::abs(p.second - expected) <= tolerance);
  }
}

}  // TEST_SUITE("POPULATION")
