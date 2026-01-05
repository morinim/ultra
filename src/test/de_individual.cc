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

#include "kernel/de/individual.h"

#include "utility/misc.h"

#include "test/fixture4.h"


#include <cstdlib>
#include <future>
#include <latch>
#include <set>
#include <sstream>

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
      const auto delta(prob.params.de.weight.sup * std::abs(a[i] - b[i]));

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
      const auto delta(prob.params.de.weight.sup * std::abs(a[i] - b[i]));

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

  SUBCASE("Calculation")
  {
    const auto cmp([](const de::individual &lhs, const de::individual &rhs)
    {
      using value_type = de::individual::value_type;

      return std::vector<value_type>(lhs) < std::vector<value_type>(rhs);
    });

    std::set<de::individual, decltype(cmp)> sample;
    std::generate_n(std::inserter(sample, sample.begin()), 200,
                    [this] { return de::individual(prob); });

    std::set<hash_t> samplehash;
    std::ranges::transform(sample, std::inserter(samplehash,
                                                 samplehash.begin()),
                           [](const auto &prg) { return prg.signature(); });

    // Distinct genomes must produce distinct signatures.
    CHECK(sample.size() == samplehash.size());
  }

  SUBCASE("Semantic consistency")
  {
    for (unsigned cycles(100); cycles; --cycles)
    {
      de::individual ind(prob), ind2(ind);

      // --- Idempotence ---
      // Calling signature() multiple times yields the same value.
      const auto s1(ind.signature());
      const auto s2(ind.signature());
      CHECK(s1 == s2);
      CHECK(!s1.empty());

      // --- Copy stability ---
      // Copying an individual preserves the signature.
      CHECK(ind.signature() == ind2.signature());

      // --- Reconstruction stability ---
      // Rebuilding from genome values preserves the signature.
      const std::vector<de::individual::value_type> vec(ind);
      de::individual ind3;

      CHECK(ind3.empty());
      ind3 = vec;
      CHECK(ind3.signature() == ind.signature());

      // --- Mutation invalidation ---
      // Any genome change must change or at least invalidate the signature.
      ind.apply([](auto &v) { v += 1.0; });
      const auto s3(ind.signature());
      CHECK(s3 != s1);

      // --- Post-mutation idempotence ---
      // After mutation, repeated calls still return the same value.
      CHECK(ind.signature() == s3);
    }
  }

  SUBCASE("Thread-safe under concurrent access")
  {
    // Increase contention.
    const auto hc(std::thread::hardware_concurrency());
    const auto threads(std::max(1u, hc * 2));

    for (unsigned cycles(1000); cycles; --cycles)
    {
      de::individual ind(prob);

      std::latch start(1);

      const auto task([&]
      {
        start.wait();
        return ind.signature();
      });

      std::vector<std::future<ultra::hash_t>> futures;
      futures.reserve(threads);

      for (unsigned i(0); i < threads; ++i)
        futures.push_back(std::async(std::launch::async, task));

      start.count_down();

      const auto ref(futures.front().get());
      CHECK(!ref.empty());

      for (std::size_t i = 1; i < futures.size(); ++i)
        CHECK(futures[i].get() == ref);
    }
  }
}

TEST_CASE_FIXTURE(fixture4, "apply")
{
  using namespace ultra;

  for (unsigned cycles(100); cycles; --cycles)
  {
    de::individual ind(prob);

    ind.apply([](auto &v) { v = std::fabs(v); });
    CHECK(std::ranges::all_of(ind, [](auto v) { return v >= 0.0; }));

    const auto half(ind.size() / 2);
    ind.apply(0, half, [](auto &v) { v = -1.0; });
    CHECK(std::ranges::count_if(ind, [](auto v) { return v < 0.0; }) == half);

    const auto s1(ind.signature());
    ind.apply(0, half, [](auto &v) { v += 1.0; });
    const auto s2(ind.signature());
    CHECK(s1 != s2);
  }
}

TEST_CASE_FIXTURE(fixture4, "Serialization")
{
  using namespace ultra;

  SUBCASE("Standard save/load")
  {
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
  }

  SUBCASE("Empty individual")
  {
    std::stringstream ss;
    de::individual empty;
    CHECK(empty.save(ss));

    de::individual empty1;
    CHECK(empty1.load(ss));
    CHECK(empty1.is_valid());
    CHECK(empty1.empty());

    CHECK(empty == empty1);
  }
}

}  // TEST_SUITE("DE INDIVIDUAL")
