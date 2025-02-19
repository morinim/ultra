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

#include "kernel/hga/problem.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("hga::problem")
{

TEST_CASE("Base")
{
  using namespace ultra;

  SUBCASE("Empty constructor")
  {
    hga::problem p;
    CHECK(p.is_valid());

    CHECK(p.parameters() == 0);

    p.insert<hga::integer>(interval{-100, 100});
    CHECK(p.parameters() == 1);

    p.insert<hga::integer>(interval{400, 500}, 0);
    CHECK(p.parameters() == 1);

    p.insert<hga::integer>(interval{-100, 100});
    CHECK(p.parameters() == 2);

    p.insert<hga::permutation>(128);
    CHECK(p.parameters() == 3);
  }
}

}  // TEST_SUITE("hga::problem")
