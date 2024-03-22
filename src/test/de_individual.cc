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
#include <set>
#include <sstream>

#include "kernel/de/individual.h"

#include "utility/misc.h"

#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("DE INDIVIDUAL")
{

TEST_CASE_FIXTURE(fixture4, "Random creation")
{
  using namespace ultra;

  for (unsigned i(0); i < 1000; ++i)
  {
    de::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(ind.parameters() == prob.sset.categories());
    CHECK(ind.age() == 0);

    for (unsigned j(0); j < ind.parameters(); ++j)
      CHECK(std::fabs(ind[j]) <= std::pow(10.0, j + 1));
  }
}

TEST_CASE_FIXTURE(fixture4, "Empty individual")
{
  ultra::de::individual ind;

  CHECK(ind.is_valid());
  CHECK(ind.empty());
}

TEST_CASE_FIXTURE(fixture4, "Comparison")
{
  using namespace ultra;

  for (unsigned i(0); i < 2000; ++i)
  {
    de::individual a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == doctest::Approx(0.0));

    de::individual b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == doctest::Approx(0.0));

    de::individual c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(!(a == c));
      CHECK(distance(a, c) > 0.0);
      CHECK(distance(a, c) == doctest::Approx(distance(c, a)));
    }
  }
}

TEST_CASE_FIXTURE(fixture4, "Iterators")
{
  using namespace ultra;

  for (unsigned j(0); j < 1000; ++j)
  {
    de::individual ind(prob);

    unsigned i(0);
    for (const auto &v : ind)
    {
      CHECK(v == doctest::Approx(ind[i]));
      ++i;
    }
  }
}

TEST_CASE_FIXTURE(fixture4, "DE crossover")
{
  using namespace ultra;

  double diff(0), length(0);

  for (unsigned j(0); j < 1000; ++j)
  {
    const de::individual p(prob);
    de::individual a(prob), b(prob), c(prob);

    a.inc_age(random::sup(100u));
    b.inc_age(random::sup(100u));
    c.inc_age(random::sup(100u));

    auto off(p.crossover(prob.params.evolution.p_cross, prob.params.de.weight,
                         p, a, a));
    CHECK(off.is_valid());

    for (unsigned i(0); i < p.parameters(); ++i)
      CHECK(off[i] == doctest::Approx(p[i]));

    off = p.crossover(prob.params.evolution.p_cross, prob.params.de.weight,
                      p, a, b);
    CHECK(off.is_valid());
    CHECK(off.age() == p.age());

    for (unsigned i(0); i < p.parameters(); ++i)
    {
      const auto delta(prob.params.de.weight.second * std::abs(a[i] - b[i]));

      CHECK(off[i] > p[i] - delta);
      CHECK(off[i] < p[i] + delta);

      if (!almost_equal(p[i], off[i]))
        ++diff;
    }

    off = p.crossover(prob.params.evolution.p_cross, prob.params.de.weight,
                      c, a, b);
    CHECK(off.is_valid());
    CHECK(off.age() == std::max({p.age(), c.age()}));
    for (unsigned i(0); i < p.parameters(); ++i)
    {
      const auto delta(prob.params.de.weight.second * std::abs(a[i] - b[i]));

      if (!almost_equal(p[i], off[i]))
      {
        CHECK(off[i] > c[i] - delta);
        CHECK(off[i] < c[i] + delta);
      }
    }

    length += p.parameters();
  }

  CHECK(diff / length < prob.params.evolution.p_cross + 2.0);
  CHECK(diff / length > prob.params.evolution.p_cross - 2.0);
}

TEST_CASE_FIXTURE(fixture4, "Signature")
{
  using namespace ultra;

  const auto cmp([](const de::individual &lhs, const de::individual &rhs)
  {
    using value_type = de::individual::value_type;

    return std::vector<value_type>(lhs) < std::vector<value_type>(rhs);
  });

  std::set<de::individual, decltype(cmp)> sample;
  std::generate_n(std::inserter(sample, sample.begin()), 200,
                  [this] { return de::individual(prob); });

  std::set<hash_t> samplehash;
  std::ranges::transform(sample, std::inserter(samplehash, samplehash.begin()),
                         [](const auto &prg) { return prg.signature(); });

  CHECK(sample.size() == samplehash.size());
}

TEST_CASE_FIXTURE(fixture4, "Serialization")
{
  using namespace ultra;

  for (unsigned i(0); i < 2000; ++i)
  {
    std::stringstream ss;

    de::individual i1(prob);
    i1.inc_age(random::sup(100u));

    CHECK(i1.save(ss));

    de::individual i2(prob);
    CHECK(i2.load(ss));
    CHECK(i2.is_valid());

    CHECK(i1 == i2);
  }

  std::stringstream ss;
  de::individual empty;
  CHECK(empty.save(ss));

  de::individual empty1;
  CHECK(empty1.load(ss));
  CHECK(empty1.is_valid());
  CHECK(empty1.empty());

  CHECK(empty == empty1);
}

}  // TEST_SUITE("DE INDIVIDUAL")
