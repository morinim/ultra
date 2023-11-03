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

#include "kernel/gp/gene.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("GENE")
{

TEST_CASE("Base")
{
  using namespace ultra;

  real::add add_inst, *add = &add_inst;

  gene g(add, {1.0, 2.0});
  CHECK(g.is_valid());
  const bool self_eq(g == g);
  CHECK(self_eq);
  CHECK(g.category() == add->category());

  const bool diff(g != gene(add, {1.0, 3.0}));
  CHECK(diff);
}

}  // TEST_SUITE("GENE")
