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

#include "kernel/layered_population.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("LAYERED POPULATION")
{

TEST_CASE_FIXTURE(fixture1, "Creation")
{
  using namespace ultra;

  prob.env.population.layers = 3;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(1, 100);

    layered_population<gp::individual> pop(prob);

    CHECK(pop.size()
          == prob.env.population.layers * prob.env.population.individuals);
    CHECK(pop.is_valid());
  }
}

TEST_CASE_FIXTURE(fixture1, "Layers and individuals")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(30, 150);
    prob.env.population.layers = random::between(1, 8);

    layered_population<gp::individual> pop(prob);

    for (std::size_t l(0); l < pop.layers(); ++l)
    {
      const auto before(pop.layer(l).size());
      const auto n(random::sup(before));

      for (unsigned j(0); j < n; ++j)
        pop.layer(l).pop_back();

      CHECK(pop.layer(l).size() == before - n);

      for (unsigned j(0); j < n; ++j)
        pop.layer(l).push_back(gp::individual(prob));

      CHECK(pop.layer(l).size() == before);
    }

    std::size_t count(std::accumulate(pop.begin(), pop.end(), 0u,
                                      [](auto acc, auto) { return ++acc; }));

    CHECK(count == pop.size());

    const unsigned added_layers(10);
    for (unsigned j(0); j < added_layers; ++j)
    {
      pop.add_layer();
      CHECK(pop.layers() == prob.env.population.layers + j + 1);
    }

    for (unsigned j(0); j < added_layers; ++j)
    {
      pop.erase(pop.layer(random::sup(pop.layers())));
      CHECK(pop.layers() == prob.env.population.layers + added_layers - j - 1);
    }

    if (pop.layers() > 1)
    {
      const auto layers(pop.range_of_layers());
      unsigned pos(0);

      for (auto it(layers.begin()); it != layers.end();)
      {
        if (pos % 2)
          it = pop.erase(it);
        else
          ++it;

        ++pos;
      }

      const auto remaining(
        static_cast<unsigned>(std::round(prob.env.population.layers / 2.0)));
      CHECK(pop.layers() == remaining);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Age")
{
  using namespace ultra;

  prob.env.population.individuals = 10;

  layered_population<gp::individual> pop(prob);

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
    prob.env.population.layers = random::between(1, 10);

    layered_population<gp::individual> pop(prob);

    CHECK(std::distance(pop.begin(), pop.end()) == pop.size());
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.env.population.individuals = random::between(10, 50);
    prob.env.population.layers = random::between(1, 4);

    std::stringstream ss;
    layered_population<gp::individual> pop1(prob);

    CHECK(pop1.save(ss));

    decltype(pop1) pop2(prob);
    CHECK(pop2.load(ss));
    CHECK(pop2.is_valid());

    CHECK(pop1.layers() == pop2.layers());
    CHECK(pop1.size() == pop2.size());
    for (std::size_t l(0); l < pop1.layers(); ++l)
      CHECK(std::ranges::equal(pop1.layer(l), pop2.layer(l)));
  }
}

TEST_CASE_FIXTURE(fixture1, "Coord")
{
  using namespace ultra;

  prob.env.population.individuals = 20;
  prob.env.population.layers = 1;

  layered_population<gp::individual> pop(prob);

  for (unsigned i(0); i < 10; ++i)
  {
    std::map<std::pair<std::size_t, linear_population<gp::individual>::coord>,
             int> frequency;

    const int draws(1000 * pop.size());
    for (int j(0); j < draws; ++j)
    {
      const auto layer(random::layer(pop));
      ++frequency[{layer, random::coord(pop.layer(layer))}];
    }

    const int expected(draws / pop.size());
    const int tolerance(16 * expected / 100);

    for (const auto &p : frequency)
      CHECK(std::abs(p.second - expected) <= tolerance);

    pop.add_layer();
  }
}

TEST_CASE_FIXTURE(fixture1, "range_of_layers")
{
  using namespace ultra;

  SUBCASE("One layer")
  {
    prob.env.population.layers = 1;
    layered_population<gp::individual> pop(prob);

    const auto range(pop.range_of_layers());
    CHECK(&*range.begin() == &pop.front());
  }

  SUBCASE("Multiple layer")
  {
    prob.env.population.layers = 4;
    layered_population<gp::individual> pop(prob);

    const auto range(pop.range_of_layers());
    for (std::size_t i(0); i < pop.layers(); ++i)
      CHECK(&*std::next(range.begin(), i) == &pop.layer(i));
  }
}

TEST_CASE_FIXTURE(fixture1, "Make debug population")
{
  using namespace ultra;
  // Individuals have distinct ages.
  const auto pop(make_debug_population<gp::individual>(prob));

  std::vector<bool> seen(pop.size(), false);

  for (const auto &prg : pop)
  {
    CHECK(!seen[prg.age()]);
    seen[prg.age()] = true;
  }
}

}  // TEST_SUITE("POPULATION")
