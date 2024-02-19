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
#include <thread>

#include "kernel/evolution_summary.h"
#include "kernel/gp/individual.h"

#include "utility/misc.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION SUMMARY")
{

TEST_CASE_FIXTURE(fixture1, "update_if_better")
{
  using namespace ultra;
  using namespace std::chrono_literals;

  summary<gp::individual, fitnd> s;

  CHECK(s.best().empty());

  const scored_individual si(gp::individual(prob), fitnd{1.0, 2.0});

  s.update_if_better(si);

  CHECK(s.best().ind == si.ind);
  CHECK(s.best().fit >= si.fit);
  CHECK(s.best().fit <= si.fit);
}

TEST_CASE_FIXTURE(fixture1, "Concurrency")
{
  using namespace ultra;

  const gp::individual dummy(prob);
  summary<gp::individual, double> sum;
  auto status1(sum.starting_status());
  auto status2(sum.starting_status());

  constexpr int MAX(1000);

  const auto work([&](bool odd)
  {
    for (int i(1); i <= MAX; ++i)
    {
      if (odd)
      {
        if (i % 2 == odd)
        {
          if (i < 8 * MAX / 10)
            status1.update_if_better({gp::individual(prob), i});
          else if (i < 9 * MAX / 10)
            status1.update_if_better({dummy, i});
        }
      }
      else  // even
      {
        if (i % 10)
          status2.update_if_better({dummy, i});
      }
    }
  });

  {
    std::jthread t1(work, false);
    std::jthread t2(work, true);
  }

  CHECK(sum.best().ind == dummy);
  CHECK(almost_equal(sum.best().fit, MAX - 1.0));

  const bool fit_exists(almost_equal(sum.best().fit, status1.best().fit)
                        || almost_equal(sum.best().fit, status2.best().fit));
  CHECK(fit_exists);

  const bool ind_exists(sum.best().ind == status1.best().ind
                        || sum.best().ind == status2.best().ind);
  CHECK(ind_exists);
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;
  using namespace std::chrono_literals;

  summary<gp::individual, fitnd> s;

  s.elapsed = 10000ms;
  s.generation = 10;
  s.score = model_measurements(fitnd{1.0, 2.0}, 0.5);

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
