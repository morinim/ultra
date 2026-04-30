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

#include "kernel/layered_population.h"
#include "kernel/gp/individual.h"

#include "test/debug_support.h"
#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <cstdlib>
#include <map>
#include <sstream>

TEST_SUITE("LAYERED POPULATION")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  CHECK(Population<layered_population<gp::individual>>);
  CHECK(LayeredPopulation<layered_population<gp::individual>>);
  CHECK(!RandomAccessIndividuals<layered_population<gp::individual>>);
  CHECK(!SizedRandomAccessPopulation<layered_population<gp::individual>>);

  CHECK(std::forward_iterator<layered_population<gp::individual>::iterator>);
  CHECK(std::sentinel_for<layered_population<gp::individual>::iterator,
                          layered_population<gp::individual>::iterator>);

  CHECK(std::forward_iterator<
        layered_population<gp::individual>::const_iterator>);
  CHECK(std::sentinel_for<layered_population<gp::individual>::const_iterator,
                          layered_population<gp::individual>::const_iterator>);
}

TEST_CASE_FIXTURE(fixture1, "Creation")
{
  using namespace ultra;

  prob.params.population.init_subgroups = 3;
  prob.params.population.min_individuals = 1;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.params.population.individuals = random::between(1, 100);

    layered_population<gp::individual> pop(prob);

    CHECK(pop.size()
          == prob.params.population.init_subgroups
             * prob.params.population.individuals);
    CHECK(pop.is_valid());
  }
}

TEST_CASE_FIXTURE(fixture1, "Layers and individuals")
{
  using namespace ultra;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.params.population.individuals    = random::between(30, 150);
    prob.params.population.init_subgroups = random::between(1, 8);

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
      CHECK(pop.layers() == prob.params.population.init_subgroups + j + 1);
    }

    for (unsigned j(0); j < added_layers; ++j)
    {
      pop.erase(pop.layer(random::sup(pop.layers())));
      CHECK(pop.layers()
            == prob.params.population.init_subgroups + added_layers - j - 1);
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
        static_cast<unsigned>(std::round(prob.params.population.init_subgroups
                                         / 2.0)));
      CHECK(pop.layers() == remaining);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "operator[]")
{
  using namespace ultra;

  prob.params.population.individuals    = 10;
  prob.params.population.init_subgroups = 3;

  layered_population<gp::individual> pop(prob);

  REQUIRE(pop.layers() == 3);

  SUBCASE("Access correctness")
  {
    for (std::size_t l(0); l < pop.layers(); ++l)
    {
      const auto &layer(pop.layer(l));

      for (std::size_t i(0); i < layer.size(); ++i)
      {
        const layered_population<gp::individual>::coord c{l, i};

        CHECK(&pop[c] == &layer[i]);
        CHECK(pop[c] == layer[i]);
      }
    }
  }

  SUBCASE("Const correctness")
  {
    const auto &cpop(pop);

    for (std::size_t l(0); l < cpop.layers(); ++l)
    {
      const auto &layer(cpop.layer(l));

      for (std::size_t i(0); i < layer.size(); ++i)
      {
        const layered_population<gp::individual>::coord c{l, i};

        CHECK(&cpop[c] == &layer[i]);
      }
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Age")
{
  using namespace ultra;

  prob.params.population.individuals = 10;

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
    prob.params.population.individuals = random::between(30, 200);
    prob.params.population.init_subgroups = random::between(1, 10);

    layered_population<gp::individual> pop(prob);

    CHECK(std::distance(pop.begin(), pop.end()) == pop.size());
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  prob.params.population.min_individuals = 1;

  for (unsigned i(0); i < 100; ++i)
  {
    prob.params.population.individuals = random::between(10, 50);
    prob.params.population.init_subgroups = random::between(1, 4);

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

TEST_CASE_FIXTURE(fixture1, "random::subgroup()")
{
  using namespace ultra;

  prob.params.population.individuals    = 20;
  prob.params.population.init_subgroups =  1;

  layered_population<gp::individual> pop(prob);

  for (unsigned i(0); i < 10; ++i)
  {
    std::map<layered_population<gp::individual>::coord, int> frequency;

    const int draws(1000 * pop.size());
    for (int j(0); j < draws; ++j)
    {
      const auto &subgroup(random::subgroup(pop));
      ++frequency[{subgroup.uid(), random::coord(subgroup)}];
    }

    const int expected(draws / pop.size());
    const int tolerance(16 * expected / 100);

    for (const auto &p : frequency)
      CHECK(std::abs(p.second - expected) <= tolerance);

    pop.add_layer();
  }
}

TEST_CASE_FIXTURE(fixture1, "random::coord()")
{
  using namespace ultra;

  SUBCASE("standard")
  {
    prob.params.population.individuals    = 20;
    prob.params.population.init_subgroups =  1;

    layered_population<gp::individual> pop(prob);

    for (unsigned i(0); i < 10; ++i)
    {
      std::map<layered_population<gp::individual>::coord, int> frequency;

      const int draws(1000 * pop.size());
      for (int j(0); j < draws; ++j)
      {
        const auto c(random::coord(pop));

        // --- Structural correctness ---
        CHECK(c.layer_index < pop.layers());
        CHECK(c.individual_coord < pop.layer(c.layer_index).size());

        ++frequency[c];
      }

      // --- Statistical correctness ---
      const int expected(draws / pop.size());
      const int tolerance(16 * expected / 100);

      for (const auto &[coord, count] : frequency)
        CHECK(std::abs(count - expected) <= tolerance);

      pop.add_layer();
    }
  }

  SUBCASE("imbalance")
  {
    prob.params.population.init_subgroups = 2;

    layered_population<gp::individual> pop(prob);

    // Force imbalance.
    pop.front().allowed(10);
    pop.back().allowed(100);

    int count_small(0);
    int count_large(0);

    for (std::size_t draws(10000); draws; --draws)
    {
      const auto [layer_i, _] = random::coord(pop);

      if (layer_i == 0) ++count_small;
      else              ++count_large;
    }

    // Should roughly match 10:100 ratio.
    const auto ratio(static_cast<double>(count_large) / count_small);
    CHECK(ratio > 8.0);
    CHECK(ratio < 12.0);
  }
}

TEST_CASE_FIXTURE(fixture1, "range_of_layers")
{
  using namespace ultra;

  SUBCASE("One layer")
  {
    prob.params.population.init_subgroups = 1;
    layered_population<gp::individual> pop(prob);

    const auto range(pop.range_of_layers());
    CHECK(&*range.begin() == &pop.front());
  }

  SUBCASE("Multiple layer")
  {
    prob.params.population.init_subgroups = 4;
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
  const auto pop(debug::make_debug_population<gp::individual>(prob));

  std::vector<bool> seen(pop.size(), false);

  for (const auto &prg : pop)
  {
    CHECK(!seen[prg.age()]);
    seen[prg.age()] = true;
  }
}

TEST_CASE_FIXTURE(fixture1, "Iterators skip empty leading layers")
{
  using namespace ultra;

  prob.params.population.individuals = 10;
  prob.params.population.init_subgroups = 3;

  layered_population<gp::individual> pop(prob);

  pop.layer(0).clear();
  pop.layer(1).clear();

  CHECK(pop.size() == pop.layer(2).size());
  CHECK(pop.begin() != pop.end());

  auto it(pop.begin());

  CHECK(it.coord().layer_index == 2);
  CHECK(it.coord().individual_coord == 0);
  CHECK(&*it == &pop.layer(2)[0]);

  CHECK(std::distance(pop.begin(), pop.end()) == pop.size());
}

TEST_CASE_FIXTURE(fixture1, "Iterators skip empty middle layers")
{
  using namespace ultra;

  prob.params.population.individuals = 5;
  prob.params.population.init_subgroups = 4;

  layered_population<gp::individual> pop(prob);

  pop.layer(1).clear();
  pop.layer(2).clear();

  std::vector<layered_population<gp::individual>::coord> coords;

  for (auto it(pop.begin()); it != pop.end(); ++it)
    coords.push_back(it.coord());

  CHECK(coords.size() == pop.size());

  for (const auto &c : coords)
  {
    CHECK(c.layer_index != 1);
    CHECK(c.layer_index != 2);
    CHECK(c.individual_coord < pop.layer(c.layer_index).size());
  }
}

TEST_CASE_FIXTURE(fixture1, "Iterator coordinates")
{
  using namespace ultra;

  prob.params.population.individuals = 7;
  prob.params.population.init_subgroups = 4;

  layered_population<gp::individual> pop(prob);

  // Create non-uniform layer sizes.
  pop.layer(0).pop_back();
  pop.layer(1).pop_back();
  pop.layer(1).pop_back();
  pop.layer(2).clear();

  std::size_t count(0);

  for (auto it(pop.begin()); it != pop.end(); ++it)
  {
    const auto c(it.coord());

    CHECK(c.layer_index < pop.layers());
    CHECK(c.individual_coord < pop.layer(c.layer_index).size());
    CHECK(&*it == &pop[c]);

    ++count;
  }

  CHECK(count == pop.size());
}

TEST_CASE_FIXTURE(fixture1, "Iterator multi-pass behaviour")
{
  using namespace ultra;

  prob.params.population.individuals = 10;
  prob.params.population.init_subgroups = 3;

  layered_population<gp::individual> pop(prob);

  auto a(pop.begin());
  auto b(a);

  CHECK(a == b);
  CHECK(&*a == &*b);

  ++a;

  CHECK(a != b);

  ++b;

  CHECK(a == b);
  CHECK(&*a == &*b);
}

TEST_CASE_FIXTURE(fixture1, "Iterator over empty population")
{
  using namespace ultra;

  prob.params.population.individuals = 5;
  prob.params.population.init_subgroups = 3;

  layered_population<gp::individual> pop(prob);

  for (auto &layer : pop.range_of_layers())
    layer.clear();

  CHECK(pop.size() == 0);
  CHECK(pop.begin() == pop.end());
  CHECK(std::distance(pop.begin(), pop.end()) == 0);
}

}  // TEST_SUITE
