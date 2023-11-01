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

#include "kernel/gp/locus.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("LOCUS")
{

TEST_CASE("Base")
{
  using namespace ultra;

  locus l1(0, 0), l2(0, 1);

  CHECK(l1 == l1);
  CHECK(l2 == l2);
  CHECK(l1 != l2);

  CHECK(l1 < l2);

  CHECK(l1 < locus::npos());
  CHECK(l2 < locus::npos());

  CHECK(l1 + 1 == locus(1,0));
  CHECK(l2 + 1 == locus(1,1));

  std::stringstream ss;
  ss << l1;
  CHECK(ss.str() == "[0,0]");
}

}  // TEST_SUITE("REAL")
