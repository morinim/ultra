/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "kernel/scored_individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SCORED INDIVIDUAL")
{

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  scored_individual<gp::individual, fitnd> si;

  CHECK(si.empty());

  std::stringstream ss;

  decltype(si) si1;

  SUBCASE("Missing best")
  {
    CHECK(si.save(ss));

    CHECK(si1.load(ss, prob));

    CHECK(si1.empty());
  }

  SUBCASE("With best")
  {
    si.ind = gp::individual(prob);
    si.fit = fitnd{1.0, 2.0};
    CHECK(si.save(ss));

    CHECK(si1.load(ss, prob));

    CHECK(si.ind == si1.ind);
    CHECK(si.fit <= si1.fit);
    CHECK(si.fit >= si1.fit);
  }
}

TEST_CASE_FIXTURE(fixture1, "Comparison")
{
  using namespace ultra;

  const scored_individual si1(gp::individual(prob), fitnd{1.0, 2.0});
  const scored_individual si2(gp::individual(prob), fitnd{2.0, 3.0});

  CHECK(si1 < si2);
  CHECK(si2 > si1);

  const decltype(si1) empty;
  CHECK(empty < si1);
  CHECK(empty < si2);
}

}  // TEST_SUITE("SCORED INDIVIDUAL")
