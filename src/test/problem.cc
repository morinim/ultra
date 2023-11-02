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

#include <sstream>

#include "kernel/problem.h"
#include "kernel/gp/primitive/real.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("PROBLEM")
{

TEST_CASE("Base")
{
  using namespace ultra;

  problem p;
  CHECK(p.is_valid());

  CHECK(p.sset.categories() == 0);
  p.insert<real::add>();
  CHECK(p.sset.categories() == 1);
}

}  // TEST_SUITE("PROBLEM")
