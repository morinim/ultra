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
  CHECK(s.last_improvement() == 0);

  const scored_individual si1(gp::individual(prob), fitnd{1.0, 2.0});
  CHECK(s.update_if_better(si1));
  CHECK(!s.update_if_better(si1));

  CHECK(s.best().ind == si1.ind);
  CHECK(almost_equal(s.best().fit, si1.fit));
  CHECK(s.last_improvement() == 0);

  const scored_individual si2(gp::individual(prob), fitnd{2.0, 3.0});
  s.generation = 2;
  CHECK(s.update_if_better(si2));

  CHECK(s.best().ind == si2.ind);
  CHECK(almost_equal(s.best().fit, si2.fit));
  CHECK(s.last_improvement() == 2);
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
      const auto di(static_cast<double>(i));

      if (odd)
      {
        if (static_cast<bool>(i % 2) == odd)
        {
          if (i < 8 * MAX / 10)
            status1.update_if_better({gp::individual(prob), di});
          else if (i < 9 * MAX / 10)
            status1.update_if_better({dummy, di});
        }
      }
      else  // even
      {
        if (i % 10)
          status2.update_if_better({dummy, di});
      }
    }
  });

  {
    std::jthread t1(work, false);
    std::jthread t2(work, true);
  }

  CHECK(sum.best().ind == dummy);
  CHECK(almost_equal(sum.best().fit, MAX - 1.0));
  CHECK(sum.last_improvement() == sum.generation);

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

  std::stringstream ss;

  decltype(s) s1;

  SUBCASE("Missing best")
  {
    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.elapsed == s1.elapsed);
    CHECK(s.generation == s1.generation);
    CHECK(s.last_improvement() == s1.last_improvement());
    CHECK(almost_equal(s.best().fit, s1.best().fit));
    CHECK(s1.best().empty());
  }

  SUBCASE("With best")
  {
    s.generation = 2;
    CHECK(s.update_if_better(
            scored_individual(gp::individual(prob), fitnd{1.0, 2.0})));

    CHECK(s.save(ss));

    CHECK(s1.load(ss, prob));

    CHECK(s.last_improvement() == s1.last_improvement());
    CHECK(s.best().ind == s1.best().ind);
    CHECK(almost_equal(s.best().fit, s1.best().fit));
  }
}

}  // TEST_SUITE("EVOLUTION SUMMARY")
