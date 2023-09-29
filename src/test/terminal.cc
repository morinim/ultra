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

#include "kernel/terminal.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("TERMINAL")
{

TEST_CASE("Double")
{
  using namespace ultra;

  terminal v("DOUBLE", 1.0);

  CHECK(v.value().index() == d_double);
  CHECK(almost_equal(std::get<D_DOUBLE>(v.value()), 1.0));
  CHECK(!v.nullary());

  terminal v1("DOUBLE", 1.000000000000000000001);

  CHECK(v == v1);
}

}  // TEST_SUITE("TERMINAL")
