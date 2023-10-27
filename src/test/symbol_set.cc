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
#include "kernel/gp/primitive/string.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SYMBOL SET")
{

TEST_CASE("Constructor / Insertion")
{
  using namespace ultra;

  symbol_set ss;

  SUBCASE("Empty symbol set")
  {
    CHECK(!ss.categories());
    CHECK(!ss.terminals());
    CHECK(ss.enough_terminals());
    CHECK(ss.is_valid());
  }

  SUBCASE("Single category symbol set")
  {
    ss.insert<real::sin>();
    ss.insert<real::cos>();
    ss.insert<real::add>();
    ss.insert<real::sub>();
    ss.insert<real::div>();
    ss.insert<real::mul>();
    CHECK(ss.categories() == 1);
    CHECK(!ss.terminals());
    CHECK(!ss.enough_terminals());

    ss.insert<real::number>();
    CHECK(ss.categories() == 1);
    CHECK(ss.terminals() == 1);
    CHECK(ss.enough_terminals());
    CHECK(ss.is_valid());
  }

  SUBCASE("Multi-category symbol set")
  {
    ss.insert<real::add>();
    ss.insert<real::number>();
    ss.insert<str::ife>(0, function::param_data_types{1, 1});
    CHECK(ss.categories() == 1);
    CHECK(ss.terminals() == 1);
    CHECK(!ss.enough_terminals());

    ss.insert<str::str>("apple", 1);
    CHECK(ss.categories() == 2);
    CHECK(ss.terminals(0) == 1);
    CHECK(ss.terminals(1) == 1);
    CHECK(ss.enough_terminals());
    CHECK(ss.is_valid());

    CHECK(ss.decode("apple"));
    CHECK(static_cast<const str::str *>(ss.decode("apple"))->instance()
          == "apple");
    CHECK(ss.decode("FADD"));
    CHECK(ss.decode("SIFE"));
    CHECK(ss.decode("REAL"));
  }
}

}  // TEST_SUITE("FUNCTION")
