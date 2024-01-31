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
  s.crossovers = 1000;
  s.mutations = 100;
  s.gen = 10;
  s.last_imp = 0;
  s.score = model_measurements(fitnd{1.0, 2.0}, 0.5);

  CHECK(s.status.best().empty());

  std::stringstream ss;

  decltype(s) s1;

  SUBCASE("Missing best")
  {
    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.elapsed == s1.elapsed);
    CHECK(s.status.crossovers == s1.status.crossovers);
    CHECK(s.status.mutations == s1.status.mutations);
    CHECK(s.gen == s1.gen);
    CHECK(s.last_imp == s1.last_imp);
    CHECK(s.score <= s1.score);
    CHECK(s.score >= s1.score);
    CHECK(s1.status.best().empty());
  }

  SUBCASE("With best")
  {
    s.status.update_if_better(
      scored_individual(gp::individual(prob), fitnd{1.0, 2.0}));

    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.status.best().ind == s1.status.best().ind);
    CHECK(s.status.best() <= s1.status.best());
    CHECK(s.status.best() >= s1.status.best());
  }
}

}  // TEST_SUITE("EVOLUTION SUMMARY")
