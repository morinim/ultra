/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <numbers>

#include "kernel/distribution.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("DISTRIBUTION")
{

TEST_CASE("Base")
{
  using namespace ultra;

  SUBCASE("Floating point")
  {
    distribution<double> d;

    CHECK(d.size() == 0);
    CHECK(d.entropy() == doctest::Approx(0.0));

    d.add(2.0);
    d.add(4.0);
    d.add(4.0);
    d.add(4.0);
    d.add(5.0);
    d.add(5.0);
    d.add(7.0);
    d.add(9.0);

    CHECK(d.size() == 8);
    CHECK(d.min() == doctest::Approx(2.0));
    CHECK(d.max() == doctest::Approx(9.0));
    CHECK(d.mean() == doctest::Approx(5.0));
    CHECK(d.variance() == doctest::Approx(4.0));
    CHECK(d.standard_deviation() == doctest::Approx(2.0));
  }

  SUBCASE("Integer")
  {
    distribution<int> d;

    CHECK(d.size() == 0);

    d.add(2);
    d.add(4);
    d.add(4);
    d.add(4);
    d.add(5);
    d.add(5);
    d.add(7);
    d.add(9);

    CHECK(d.size() == 8);
    CHECK(d.min() == 2);
    CHECK(d.max() == 9);
    CHECK(d.mean() == 5);
    CHECK(d.variance() == 4);
    CHECK(d.standard_deviation() == 2);
  }
}

}  // TEST_SUITE("FUNCTION")
