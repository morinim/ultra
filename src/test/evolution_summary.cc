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

#include "kernel/evolution_summary.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION SUMMARY")
{

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;
  using namespace std::chrono_literals;

  summary<gp::individual, fitnd> s;

  s.elapsed = 10000ms;
  s.generation = 10;
  s.score = model_measurements(fitnd{1.0, 2.0}, 0.5);

  CHECK(s.best().empty());

  std::stringstream ss;

  decltype(s) s1;

  SUBCASE("Missing best")
  {
    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.elapsed == s1.elapsed);
    CHECK(s.generation == s1.generation);
    CHECK(s.score <= s1.score);
    CHECK(s.score >= s1.score);
    CHECK(s1.best().empty());
  }

  SUBCASE("With best")
  {
    s.update_if_better(
      scored_individual(gp::individual(prob), fitnd{1.0, 2.0}));

    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.best().ind == s1.best().ind);
    CHECK(s.best() <= s1.best());
    CHECK(s.best() >= s1.best());
  }
}

}  // TEST_SUITE("EVOLUTION SUMMARY")
