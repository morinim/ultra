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

#include <numbers>

#include "kernel/distribution.h"
#include "kernel/random.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

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

TEST_CASE("Merge")
{
  using namespace ultra;

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

      if (cycles < 500)
        d1.add(elem);
      else
        d2.add(elem);
    }

    CHECK(d1.min() >= -1000.0);
    CHECK(d1.max() < 1000.0);

    d1.merge(std::move(d2));

    CHECK(d.mean() == doctest::Approx(d1.mean()));
    CHECK(d.min() == doctest::Approx(d1.min()));
    CHECK(d.max() == doctest::Approx(d1.max()));
    CHECK(d.size() == d1.size());
    CHECK(d.variance() == doctest::Approx(d1.variance()));
  }
}

TEST_CASE("Serialization")
{
  using namespace ultra;
  distribution<double> d;

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

  CHECK(rif_min == doctest::Approx(d.min()));
  CHECK(rif_max == doctest::Approx(d.max()));
  CHECK(rif_mean == doctest::Approx(d.mean()));
  CHECK(rif_variance == doctest::Approx(d.variance()));
  CHECK(d.seen() == d1.seen());
}

}  // TEST_SUITE("FUNCTION")
