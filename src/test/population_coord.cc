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

#include <cstdlib>

#include "kernel/population.h"
#include "kernel/random.h"
#include "kernel/gp/individual.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("POPULATION_COORD")
{

TEST_CASE("Comparison")
{
  using namespace ultra;

  using coord = population<gp::individual>::coord;

  const coord c{random::between(0u, 100u), random::between(0u, 100u)};
  const auto c1(c);

  CHECK(c == c1);
  CHECK(!(c < c1));
  CHECK(!(c1 < c));

  auto c2(c);
  ++c2.layer;
  CHECK(c != c2);
  CHECK(c < c2);

  --c2.index;
  CHECK(c != c2);
  CHECK(c < c2);

  c2 = c;
  ++c2.index;
  CHECK(c != c2);
  CHECK(c < c2);
}

}  // TEST_SUITE("POPULATION_COORD")
