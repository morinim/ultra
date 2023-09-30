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

#include "kernel/value.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("VALUE_T")
{

TEST_CASE("Base")
{
  using namespace ultra;

  value_t v1;
  CHECK(!has_value(v1));
  CHECK(v1.index() == d_void);
  const bool self_eq(v1 == v1);
  CHECK(self_eq);

  v1 = std::string("dummy");
  CHECK(has_value(v1));
  CHECK(v1.index() == d_string);
  const bool string_eq(v1 == v1);
  CHECK(string_eq);

  v1 = [] { std::cout << "Hello world\n"; };
  CHECK(has_value(v1));
  CHECK(v1.index() == d_nullary);
  const bool nullary_eq(v1 == v1);
  CHECK(nullary_eq);

  v1 = 1;
  CHECK(has_value(v1));
  CHECK(v1.index() == d_int);
  const bool int_eq(v1 == v1);
  CHECK(int_eq);

  v1 = 1.0;
  CHECK(has_value(v1));
  CHECK(v1.index() == d_double);
  const bool double_eq(v1 == v1);
  CHECK(double_eq);

  value_t v2(1.00000000000001);
  CHECK(has_value(v2));
  CHECK(v2.index() == d_double);
  const bool almost_eq(v1 == v2);
  CHECK(almost_eq);

  v1 = 1;
  CHECK(has_value(v1));
  CHECK(v1.index() == d_int);
  const bool type_diff(v1 != v2);
  CHECK(type_diff);
}

}  // TEST_SUITE("VALUE_T")
