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

#include "kernel/distribution.h"
#include "kernel/random.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <numbers>

TEST_SUITE("DISTRIBUTION")
{

TEST_CASE("Base")
{
  using namespace ultra;

  distribution<double> d;

  CHECK(d.size() == 0);

  const std::vector<std::pair<double, unsigned>> elems =
  {
    {2.0, 1},
    {4.0, 3},
    {5.0, 2},
    {7.0, 1},
    {9.0, 1}
  };

  for (const auto &e : elems)
    for (unsigned n(e.second); n; --n)
      d.add(e.first);

  const auto added(std::accumulate(
                     elems.begin(), elems.end(),0,
                     [](auto sum, const auto &v) { return sum + v.second; }));

  CHECK(d.size() == added);

  d.add(std::numeric_limits<double>::quiet_NaN());
  CHECK(d.size() == added);

  CHECK(d.min() == doctest::Approx(2.0));
  CHECK(d.max() == doctest::Approx(9.0));
  CHECK(d.mean() == doctest::Approx(5.0));
  CHECK(d.variance() == doctest::Approx(4.0));
  CHECK(d.standard_deviation() == doctest::Approx(2.0));


  for (const auto &e : elems)
  {
    const auto it(d.seen().find(e.first));
    CHECK(it != d.seen().end());

    CHECK(it->second == e.second);
  }

  const auto e1(d.entropy());

  for (const auto &e : elems)
  {
    d.add(e.first + 0.000001);

    const auto it(d.seen().find(e.first));
    CHECK(it != d.seen().end());

    CHECK(it->second == e.second + 1);
  }

  const auto e2(d.entropy());

  CHECK(d.size() == added + elems.size());

  CHECK(e1 < e2);

  d.add(7.0);
  d.add(9.0);

  CHECK(e2 < d.entropy());
}

TEST_CASE("Empty state")
{
  using namespace ultra;

  distribution<double> d;

  CHECK(d.empty());
  CHECK(d.size() == 0);
  CHECK(d.seen().empty());
  CHECK(d.entropy() == doctest::Approx(0.0));
  CHECK(d.is_valid());

  d.add(1.0);
  d.add(2.0);
  d.clear();

  CHECK(d.empty());
  CHECK(d.size() == 0);
  CHECK(d.seen().empty());
  CHECK(d.entropy() == doctest::Approx(0.0));
  CHECK(d.is_valid());

  std::stringstream s;
  CHECK(d.save(s));

  distribution<double> loaded;
  loaded.add(42.0);

  CHECK(loaded.load(s));
  CHECK(loaded.empty());
  CHECK(loaded.size() == 0);
  CHECK(loaded.seen().empty());
  CHECK(loaded.entropy() == doctest::Approx(0.0));
  CHECK(loaded.is_valid());
}

TEST_CASE("Single element state")
{
  using namespace ultra;

  for (const double val : {42.5, -123.456, 0.0, 1e9, -1e9})
  {
    distribution<double> d;
    d.add(val);

    CHECK_FALSE(d.empty());
    CHECK(d.size() == 1);
    CHECK(d.min() == doctest::Approx(val));
    CHECK(d.max() == doctest::Approx(val));
    CHECK(d.mean() == doctest::Approx(val));
    CHECK(d.variance() == doctest::Approx(0.0));
    CHECK(d.standard_deviation() == doctest::Approx(0.0));
    CHECK(d.entropy() == doctest::Approx(0.0));
    CHECK(d.seen().size() == 1);
    CHECK(d.seen().contains(round_to(val)));
    CHECK(d.seen().at(round_to(val)) == 1);
    CHECK(d.is_valid());

    std::stringstream s;
    CHECK(d.save(s));

    distribution<double> loaded;
    CHECK(loaded.load(s));
    CHECK_FALSE(loaded.empty());
    CHECK(loaded.size() == 1);
    CHECK(loaded.min() == doctest::Approx(val));
    CHECK(loaded.max() == doctest::Approx(val));
    CHECK(loaded.mean() == doctest::Approx(val));
    CHECK(loaded.variance() == doctest::Approx(0.0));
    CHECK(loaded.standard_deviation() == doctest::Approx(0.0));
    CHECK(loaded.entropy() == doctest::Approx(0.0));
    CHECK(loaded.seen() == d.seen());
    CHECK(loaded.is_valid());
  }
}

TEST_CASE("Merge")
{
  using namespace ultra;

  ultra::random::engine().seed(1973u);

  SUBCASE("Same distribution")
  {
    const std::vector<std::pair<double, unsigned>> elems =
    {
      {2.0, 1},
      {4.0, 3},
      {5.0, 2},
      {7.0, 1},
      {9.0, 1}
    };

    distribution<double> d;
    for (const auto &e : elems)
      for (unsigned n(e.second); n; --n)
        d.add(e.first);

    const auto mean_before(d.mean());
    const auto variance_before(d.variance());
    const auto min_before(d.min());
    const auto max_before(d.max());
    const auto size_before(d.size());

    auto d2(d);

    d.merge(std::move(d2));

    CHECK(d.mean() == doctest::Approx(mean_before));
    CHECK(d.min() == doctest::Approx(min_before));
    CHECK(d.max() == doctest::Approx(max_before));
    CHECK(d.size() == doctest::Approx(2 * size_before));
    CHECK(d.variance() == doctest::Approx(variance_before));
  }

  SUBCASE("Single element distribution")
  {
    distribution<double> d1;
    d1.add(-1.0);

    distribution<double> d2;
    d2.add(+1.0);

    d1.merge(std::move(d2));

    CHECK(d1.mean() == doctest::Approx(0.0));
    CHECK(d1.variance() == doctest::Approx(1.0));
    CHECK(d1.min() == doctest::Approx(-1.0));
    CHECK(d1.size() == 2);
    CHECK(d1.max() == doctest::Approx(+1.0));
  }

  SUBCASE("General case")
  {
    distribution<double> d, d1, d2;

    for (unsigned cycles(100); cycles; --cycles)
    {
      const auto elem(random::between(-1000.0, 1000.0));
      d.add(elem);

      if (cycles % 2)
        d1.add(elem);
      else
        d2.add(elem);
    }

    CHECK(d1.min() >= -1000.0);
    CHECK(d1.max() < 1000.0);

    CHECK(d2.min() >= -1000.0);
    CHECK(d2.max() < 1000.0);

    d1.merge(std::move(d2));

    CHECK(d.mean() == doctest::Approx(d1.mean()));
    CHECK(d.min() == doctest::Approx(d1.min()));
    CHECK(d.max() == doctest::Approx(d1.max()));
    CHECK(d.size() == d1.size());
    CHECK(d.variance() == doctest::Approx(d1.variance()));
    CHECK(d.seen() == d1.seen());
  }

  SUBCASE("Empty distribution")
  {
    distribution<double> d, d1, empty;

    for (unsigned cycles(100); cycles; --cycles)
    {
      const auto elem(random::between(-1000.0, 1000.0));
      d.add(elem);
      d1.add(elem);
    }

    SUBCASE("Non empty - merge - empty")
    {
      d1.merge(empty);

      CHECK(d.mean() == doctest::Approx(d1.mean()));
      CHECK(d.min() == doctest::Approx(d1.min()));
      CHECK(d.max() == doctest::Approx(d1.max()));
      CHECK(d.size() == d1.size());
      CHECK(d.variance() == doctest::Approx(d1.variance()));
      CHECK(d.seen() == d1.seen());
    }

    SUBCASE("Empty - merge - non empty")
    {
      empty.merge(d1);

      CHECK(d.mean() == doctest::Approx(empty.mean()));
      CHECK(d.min() == doctest::Approx(empty.min()));
      CHECK(d.max() == doctest::Approx(empty.max()));
      CHECK(d.size() == empty.size());
      CHECK(d.variance() == doctest::Approx(empty.variance()));
      CHECK(d.seen() == empty.seen());
    }
  }

  SUBCASE("Large mean small variance")
  {
    const double base(1e9);

    distribution<double> d, d1, d2;

    d.add(base + 1.0);
    d.add(base - 1.0);
    d.add(base + 2.0);
    d.add(base - 2.0);

    d1.add(base + 1.0);
    d1.add(base - 1.0);

    d2.add(base + 2.0);
    d2.add(base - 2.0);

    d1.merge(d2);

    CHECK(d1.is_valid());
    CHECK(d1.mean() == doctest::Approx(d.mean()));
    CHECK(d1.variance() == doctest::Approx(d.variance()));
    CHECK(d1.variance() == doctest::Approx(2.5));
    CHECK(d1.min() == doctest::Approx(d.min()));
    CHECK(d1.max() == doctest::Approx(d.max()));
    CHECK(d1.size() == d.size());
    CHECK(d1.seen() == d.seen());

    distribution<double> d3, d4;
    d3.add(base);
    d3.add(base);
    d4.add(base);
    d4.add(base);
    d3.merge(d4);

    CHECK(d3.is_valid());
    CHECK(d3.mean() == doctest::Approx(base));
    CHECK(d3.variance() == doctest::Approx(0.0));

    // Asymmetric sizes: size1 < size2 and size1 > size2
    distribution<double> large, small;
    for (unsigned i(0); i < 1000; ++i)
      large.add(base);
    small.add(base + 1.0);

    auto m_large_small(large);
    m_large_small.merge(small);

    auto m_small_large(small);
    m_small_large.merge(large);

    CHECK(m_large_small.is_valid());
    CHECK(m_small_large.is_valid());
    CHECK(m_large_small.mean() == doctest::Approx(m_small_large.mean()));
    CHECK(m_large_small.variance() == doctest::Approx(m_small_large.variance()));
  }

  SUBCASE("Large negative mean small variance")
  {
    const double base(-1e9);

    distribution<double> d, d1, d2;

    d.add(base + 1.0);
    d.add(base - 1.0);
    d.add(base + 2.0);
    d.add(base - 2.0);

    d1.add(base + 1.0);
    d1.add(base - 1.0);

    d2.add(base + 2.0);
    d2.add(base - 2.0);

    d1.merge(d2);

    CHECK(d1.is_valid());
    CHECK(d1.mean() == doctest::Approx(d.mean()));
    CHECK(d1.mean() == doctest::Approx(base));
    CHECK(d1.variance() == doctest::Approx(d.variance()));
    CHECK(d1.variance() == doctest::Approx(2.5));
    CHECK(d1.min() == doctest::Approx(d.min()));
    CHECK(d1.max() == doctest::Approx(d.max()));
    CHECK(d1.size() == d.size());
    CHECK(d1.seen() == d.seen());

    distribution<double> d3, d4;
    d3.add(base);
    d3.add(base);
    d4.add(base);
    d4.add(base);
    d3.merge(d4);

    CHECK(d3.is_valid());
    CHECK(d3.mean() == doctest::Approx(base));
    CHECK(d3.variance() == doctest::Approx(0.0));

    // Asymmetric sizes: size1 < size2 and size1 > size2 with negative values
    distribution<double> large, small;
    for (unsigned i(0); i < 1000; ++i)
      large.add(base);
    small.add(base - 1.0);

    auto m_large_small(large);
    m_large_small.merge(small);

    auto m_small_large(small);
    m_small_large.merge(large);

    CHECK(m_large_small.is_valid());
    CHECK(m_small_large.is_valid());
    CHECK(m_large_small.mean() == doctest::Approx(m_small_large.mean()));
    CHECK(m_large_small.variance() == doctest::Approx(m_small_large.variance()));

    // Cross-sign merge: one large positive, one large negative
    distribution<double> pos, neg;
    pos.add(1e9);
    neg.add(-1e9);
    pos.merge(neg);

    CHECK(pos.is_valid());
    CHECK(pos.size() == 2);
    CHECK(pos.mean() == doctest::Approx(0.0));
    CHECK(pos.variance() == doctest::Approx(1e18));
  }

  SUBCASE("Add after merge")
  {
    distribution<double> d1, d2, d_all;

    d1.add(10.0);
    d1.add(20.0);
    d2.add(30.0);
    d2.add(40.0);

    for (double v : {10.0, 20.0, 30.0, 40.0, 50.0})
      d_all.add(v);

    d1.merge(d2);
    d1.add(50.0);  // add after merge

    CHECK(d1.is_valid());
    CHECK(d1.size() == d_all.size());
    CHECK(d1.mean() == doctest::Approx(d_all.mean()));
    CHECK(d1.variance() == doctest::Approx(d_all.variance()));
  }

  SUBCASE("Chained merges")
  {
    distribution<double> combined, all_direct;

    for (int chunk(0); chunk < 5; ++chunk)
    {
      distribution<double> sub;
      for (int i(0); i < 100; ++i)
      {
        const double v(chunk * 100.0 + i);
        sub.add(v);
        all_direct.add(v);
      }

      combined.merge(sub);
    }

    CHECK(combined.is_valid());
    CHECK(combined.size() == all_direct.size());
    CHECK(combined.mean() == doctest::Approx(all_direct.mean()));
    CHECK(combined.variance() == doctest::Approx(all_direct.variance()));
  }

  SUBCASE("Self-merge aliasing")
  {
    distribution<double> d;
    d.add(1.0);
    d.add(3.0);

    const auto original_mean = d.mean();
    const auto original_var = d.variance();

    d.merge(d);

    CHECK(d.is_valid());
    CHECK(d.size() == 4);
    CHECK(d.mean() == doctest::Approx(original_mean));
    CHECK(d.variance() == doctest::Approx(original_var));
  }
}

TEST_CASE("Single-precision float")
{
  ultra::distribution<float> df1, df2;
  df1.add(1.0f);
  df1.add(2.0f);
  df2.add(3.0f);
  df2.add(4.0f);

  df1.merge(df2);

  CHECK(df1.is_valid());
  CHECK(df1.size() == 4);
  CHECK(df1.mean() == doctest::Approx(2.5f));
  CHECK(df1.variance() == doctest::Approx(1.25f));
}

TEST_CASE("Serialization")
{
  using namespace ultra;
  distribution<double> d;

  SUBCASE("Base")
  {
    ultra::random::engine().seed(1973u);

    for (unsigned i(0); i < 10000; ++i)
      d.add(random::sup(10.0));

    const auto rif_min(d.min());
    const auto rif_max(d.max());
    const auto rif_mean(d.mean());
    const auto rif_variance(d.variance());

    CHECK(4.5 <= rif_mean);
    CHECK(rif_mean <= 5.5);

    std::stringstream s;
    CHECK(d.save(s));

    decltype(d) d1;
    CHECK(d1.load(s));

    CHECK(rif_min == doctest::Approx(d1.min()));
    CHECK(rif_max == doctest::Approx(d1.max()));
    CHECK(rif_mean == doctest::Approx(d1.mean()));
    CHECK(rif_variance == doctest::Approx(d1.variance()));
    CHECK(d.seen() == d1.seen());
  }

  SUBCASE("Corrupted stream load")
  {
    d.add(5.0);
    d.add(10.0);

    const auto prev_mean(d.mean());
    const auto prev_size(d.size());

    std::stringstream corrupt("corrupted gibberish data");
    CHECK_FALSE(d.load(corrupt));

    // Verify object was not modified.
    CHECK(d.size() == prev_size);
    CHECK(d.mean() == doctest::Approx(prev_mean));
    CHECK(d.is_valid());
  }
}

TEST_CASE("Exact entropy")
{
  ultra::distribution<double> d;
  CHECK(d.entropy() == doctest::Approx(0.0));

  d.add(10.0);
  CHECK(d.entropy() == doctest::Approx(0.0));

  d.add(20.0);  // 2 bins, equal counts
  CHECK(d.entropy() == doctest::Approx(1.0));

  d.add(30.0);
  d.add(40.0);  // 4 bins, equal counts
  CHECK(d.entropy() == doctest::Approx(2.0));
}

}  // TEST_SUITE
