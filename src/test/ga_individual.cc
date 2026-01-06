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

#include "kernel/ga/individual.h"

#include "test/fixture5.h"

#include <cstdlib>
#include <future>
#include <iterator>
#include <latch>
#include <set>
#include <sstream>

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

  SUBCASE("Mutation sequences")
  {
    std::vector<double> sequence;
    for (double pgm(0.1); pgm <= 1.0; pgm += 0.1)
    {
      prob.params.evolution.p_mutation = pgm;

      unsigned diff(0);
      for (unsigned i(0); i < n; ++i)
      {
        auto i1(orig);

        i1.mutation(prob);
        diff += distance(orig, i1);
      }

      sequence.push_back(diff);
    }

    CHECK(std::ranges::is_sorted(sequence));
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

TEST_CASE("Distance")
{
  using namespace ultra;

  ga::problem prob;
  prob.params.init();

  prob.insert(interval(0, 9));
  prob.insert(interval(0, 9));
  prob.insert(interval(0, 9));
  prob.insert(interval(0, 9));

  ga::individual a(prob), b(prob);

  a = std::vector{0, 1, 2, 3};
  b = std::vector{0, 2, 2, 2};

  CHECK(distance(a, b) == 2);
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

TEST_CASE_FIXTURE(fixture5, "apply")
{
  using namespace ultra;

  for (unsigned cycles(100); cycles; --cycles)
  {
    ga::individual ind(prob);

    ind.apply_each([](auto &v) { v = std::fabs(v); });
    CHECK(std::ranges::all_of(ind, [](auto v) { return v >= 0.0; }));

    const auto half(ind.size() / 2);
    ind.apply_each(0, half, [](auto &v) { v = -1.0; });
    CHECK(std::ranges::count_if(ind, [](auto v) { return v < 0.0; }) == half);

    const auto s1(ind.signature());
    ind.apply_each(0, half, [](auto &v) { v += 1.0; });
    const auto s2(ind.signature());
    CHECK(s1 != s2);
  }
}

TEST_CASE_FIXTURE(fixture5, "Signature")
{
  using namespace ultra;

  SUBCASE("Calculation")
  {
    const auto cmp([](const ga::individual &lhs, const ga::individual &rhs)
    {
      using value_type = ga::individual::value_type;

      return std::vector<value_type>(lhs) < std::vector<value_type>(rhs);
    });

    std::set<ga::individual, decltype(cmp)> sample;
    std::generate_n(std::inserter(sample, sample.begin()), 200,
                    [this] { return ga::individual(prob); });

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
      ga::individual ind(prob), ind2(ind);

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
      const std::vector<ga::individual::value_type> vec(ind);
      ga::individual ind3;

      CHECK(ind3.empty());
      ind3 = vec;
      CHECK(ind3.signature() == ind.signature());

      // --- Mutation invalidation ---
      // Any genome change must change or at least invalidate the signature.
      ind.apply_each([](auto &v) { v += 1.0; });
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
      ga::individual ind(prob);

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

TEST_CASE_FIXTURE(fixture5, "Serialisation")
{
  SUBCASE("Standard save/load sequence")
  {
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
  }

  SUBCASE("Empty individual")
  {
    std::stringstream ss;
    ultra::ga::individual empty;
    CHECK(empty.save(ss));

    ultra::ga::individual empty1;
    CHECK(empty1.load(ss));
    CHECK(empty1.is_valid());
    CHECK(empty1.empty());

    CHECK(empty == empty1);
  }
}

}  // TEST_SUITE("ga::individual")
