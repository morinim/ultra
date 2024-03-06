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

  for (const auto &e : elems)
  {
    d.add(e.first + 0.000001);

    const auto it(d.seen().find(e.first));
    CHECK(it != d.seen().end());

    CHECK(it->second == e.second + 1);
  }

  CHECK(d.size() == added + elems.size());

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
