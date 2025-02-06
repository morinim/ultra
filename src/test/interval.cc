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

#include <sstream>
#include <vector>

#include "kernel/interval.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("INTERVAL")
{

TEST_CASE("Comparison")
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

  SUBCASE("Integral")
  {
    const int min(0), sup(10);

    const interval i(min, sup);
    CHECK(i.min == min);
    CHECK(i.sup == sup);

    CHECK(i.is_valid());
  }

  SUBCASE("")
  {
    const int min(0);
    const std::size_t sup(10);

    const interval<int> i({min, sup});
    CHECK(i.min == min);
    CHECK(i.sup == sup);

    CHECK(i.is_valid());
  }
}

}  // TEST_SUITE("INTERVAL")
