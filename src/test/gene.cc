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

  gene uninitialized;
  const bool unknown(uninitialized.is_valid() || !uninitialized.is_valid());
  CHECK(unknown);

  gene g(add, {1.0, 2.0});
  CHECK(g.is_valid());
  const bool self_eq(g == g);
  CHECK(self_eq);

  const bool diff(g != uninitialized);
  CHECK(diff);

  CHECK(g.category() == add->category());
}

}  // TEST_SUITE("GENE")
