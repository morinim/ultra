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

#include <sstream>

#include "kernel/de/problem.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("de::problem")
{

TEST_CASE("Base")
{
  using namespace ultra;

  SUBCASE("Empty constructor")
  {
    de::problem p;
    CHECK(p.is_valid());

    CHECK(p.parameters() == 0);
    p.insert({-100.0, 100.0});
    CHECK(p.parameters() == 1);
    p.insert({400.0, 500.0}, 0);
    CHECK(p.parameters() == 1);
    p.insert({-100.0, 100.0});
    CHECK(p.parameters() == 2);
  }

  SUBCASE("Base constructor")
  {
    de::problem p(4, {-1000.0, 1000.0});
    CHECK(p.is_valid());

    CHECK(p.parameters() == 4);
  }

  SUBCASE("Advanced constructor")
  {
    de::problem p({{-1000.0, 1000.0}, {0.0, 10.0}, {0.0, 100.0}});
    CHECK(p.is_valid());

    CHECK(p.parameters() == 3);
  }
}

}  // TEST_SUITE("de::problem")
