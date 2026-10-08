/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/interval.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <sstream>
#include <utility>
#include <vector>

TEST_SUITE("INTERVAL")
{

TEST_CASE("Construction")
{
  using namespace ultra;

  SUBCASE("Floating point")
  {
    const double min(0.0), sup(10.0);

    const interval i(min, sup);
    CHECK(i.min == doctest::Approx(min));
    CHECK(i.sup == doctest::Approx(sup));

    CHECK(i.is_valid());
  }

  SUBCASE("Floating point from integer literals")
  {
    const interval<double> i(0, 10);
    CHECK(i.min == doctest::Approx(0.0));
    CHECK(i.sup == doctest::Approx(10.0));

    CHECK(i.is_valid());
  }

  SUBCASE("Integral")
  {
    const int min(0), sup(10);

    const interval i(min, sup);
    CHECK(i.min == min);
    CHECK(i.sup == sup);

    CHECK(i.is_valid());
  }

  SUBCASE("Pair")
  {
    const int min(0);
    const std::size_t sup(10);

    const interval<int> i({min, sup});
    CHECK(i.min == min);
    CHECK(i.sup == sup);

    CHECK(i.is_valid());

    const std::pair pi(min, sup);
    const interval<int> ip(pi);
    CHECK(ip.min == min);
    CHECK(ip.sup == sup);

    CHECK(ip.is_valid());

    const double min_d(0.0), sup_d(10.0);
    const std::pair pd(min_d, sup_d);
    const interval<double> id(pd);
    CHECK(id.min == doctest::Approx(min_d));
    CHECK(id.sup == doctest::Approx(sup_d));

    CHECK(id.is_valid());
  }
}

TEST_CASE("Comparison")
{
  using namespace ultra;

  const interval i1(0, 10);
  const interval i2(0, 10);
  const interval i3(1, 10);
  const interval i4(0, 11);

  CHECK(i1 == i2);
  CHECK(i2 == i1);
  CHECK(i1 != i3);
  CHECK(i3 != i1);
  CHECK(i1 != i4);
  CHECK(i4 != i1);

  static_assert(interval(0, 10) == interval(0, 10));
  static_assert(interval(0, 10) != interval(1, 10));
  static_assert(interval(0, 10) != interval(0, 11));
}

}  // TEST_SUITE("INTERVAL")
