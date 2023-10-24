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

#include "kernel/symbol_set.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SYMBOL SET")
{

TEST_CASE("Base")
{
  using namespace ultra;

  symbol_set ss;

  ss.insert<real::sin>();
  ss.insert<real::cos>();
  ss.insert<real::add>();
  ss.insert<real::sub>();
  ss.insert<real::div>();
  ss.insert<real::mul>();
  ss.insert<real::number>();

  CHECK(ss.categories() == 1);
}

}  // TEST_SUITE("FUNCTION")
