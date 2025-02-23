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
#include <sstream>

#include "kernel/ga/individual.h"

#include "test/fixture5.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("ga::individual")
{

TEST_CASE_FIXTURE(fixture5, "Random creation")
{
  for (unsigned cycles(1000); cycles; --cycles)
  {
    ultra::ga::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(!ind.empty());
    CHECK(ind.parameters() == prob.sset.categories());
    CHECK(ind.age() == 0);

    for (std::size_t j(0); j < ind.parameters(); ++j)
    {
      CHECK(intervals[j].is_valid());
      CHECK(intervals[j].min <= ind[j]);
      CHECK(ind[j] < intervals[j].sup);
    }
  }
}

TEST_CASE_FIXTURE(fixture5, "Empty individual")
{
  ultra::ga::individual ind;

  CHECK(ind.is_valid());
  CHECK(ind.empty());
}

TEST_CASE_FIXTURE(fixture5, "Mutation")
{
  ultra::ga::individual t(prob);
  const auto orig(t);

  const unsigned n(1000);

  SUBCASE("Zero probability mutation")
  {
    prob.params.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < n; ++i)
    {
      t.mutation(prob);
      CHECK(t == orig);
    }
  }

  SUBCASE("50% probability mutation")
  {
    unsigned diff(0);

    prob.params.evolution.p_mutation = 0.5;

    for (unsigned i(0); i < n; ++i)
    {
      auto i1(orig);

      i1.mutation(prob);
      diff += distance(orig, i1);
    }

    const double perc(100.0 * double(diff) / double(orig.parameters() * n));
    CHECK(perc > 47.0);
    CHECK(perc < 53.0);
  }
}

TEST_CASE_FIXTURE(fixture5, "Comparison")
{
  for (unsigned cycles(2000); cycles; --cycles)
  {
    ultra::ga::individual a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == 0);

    auto b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    ultra::ga::individual c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(!(a == c));
      CHECK(distance(a, c) > 0);
      CHECK(distance(a, c) == distance(c, a));
    }
  }
}

TEST_CASE_FIXTURE(fixture5, "Iterators")
{
  for (unsigned cycles(1000); cycles; --cycles)
  {
    ultra::ga::individual ind(prob);

    for (unsigned i(0); const auto &g : ind)
      CHECK(g == ind[i++]);
  }
}

TEST_CASE_FIXTURE(fixture5, "Standard crossover")
{
  ultra::ga::individual i1(prob), i2(prob);

  for (unsigned cycles(1000); cycles; --cycles)
  {
    if (ultra::random::boolean())
      i1.inc_age();
    if (ultra::random::boolean())
      i2.inc_age();

    const auto ic(crossover(prob, i1, i2));
    CHECK(ic.is_valid());
    CHECK(ic.age() == std::max(i1.age(), i2.age()));

    const auto d1(distance(i1, ic));
    CHECK(0 <= d1);
    CHECK(d1 <= i1.parameters());

    const auto d2(distance(i2, ic));
    CHECK(0 <= d2);
    CHECK(d1 <= i2.parameters());

    for (std::size_t k(0); k < ic.parameters(); ++k)
    {
      const bool from_1_or_2(ic[k] == i1[k] || ic[k] == i2[k]);
      CHECK(from_1_or_2);
    }
  }
}

TEST_CASE_FIXTURE(fixture5, "Serialization")
{
  // Non-empty ga::individual serialization.
  for (unsigned cycles(2000); cycles; --cycles)
  {
    std::stringstream ss;
    ultra::ga::individual i1(prob);

    i1.inc_age(ultra::random::sup(100));

    CHECK(i1.save(ss));

    ultra::ga::individual i2(prob);
    CHECK(i2.load(ss));
    CHECK(i2.is_valid());

    CHECK(i1 == i2);
  }

  // Non-empty ga::individual serialization.
  std::stringstream ss;
  ultra::ga::individual empty;
  CHECK(empty.save(ss));

  ultra::ga::individual empty1;
  CHECK(empty1.load(ss));
  CHECK(empty1.is_valid());
  CHECK(empty1.empty());

  CHECK(empty == empty1);
}

}  // TEST_SUITE("ga::individual")
