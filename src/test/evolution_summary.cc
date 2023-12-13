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
  s.crossovers = 1000;
  s.mutations = 100;
  s.gen = 10;
  s.last_imp = 0;

  CHECK(s.best.solution.empty());

  std::stringstream ss;

  decltype(s) s1;

  SUBCASE("Missing best")
  {
    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.elapsed == s1.elapsed);
    CHECK(s.crossovers == s1.crossovers);
    CHECK(s.mutations == s1.mutations);
    CHECK(s.gen == s1.gen);
    CHECK(s.last_imp == s1.last_imp);
    CHECK(s1.best.solution.empty());
  }

  SUBCASE("With best")
  {
    s.best.solution = gp::individual(prob);
    s.best.score = model_measurements(fitnd{1.0, 2.0}, 0.5);
    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.best.solution == s1.best.solution);
    CHECK(s.best.score <= s1.best.score);
    CHECK(s.best.score >= s1.best.score);
  }
}

}  // TEST_SUITE("EVOLUTION SUMMARY")
