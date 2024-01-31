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
#include <thread>
#include <sstream>

#include "kernel/evolution_status.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION STATUS")
{

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Missing best")
  {
    evolution_status<gp::individual, fitnd> status;
    status.crossovers = 1000;
    status.mutations = 100;

    CHECK(status.best().empty());

    CHECK(status.save(ss));

    decltype(status) status1;
    CHECK(status1.load(ss, prob));

    CHECK(status.crossovers == status1.crossovers);
    CHECK(status.mutations == status1.mutations);
    CHECK(status1.best().empty());
  }

  SUBCASE("With best")
  {
    evolution_status<gp::individual, fitnd> status(
      scored_individual(gp::individual(prob), fitnd{1.0, 2.0}));
    status.crossovers = 1000;
    status.mutations = 100;

    CHECK(!status.best().empty());

    CHECK(status.save(ss));

    decltype(status) status1;
    CHECK(status1.load(ss, prob));

    CHECK(status.crossovers == status1.crossovers);
    CHECK(status.mutations == status1.mutations);

    const auto best(status.best()), best1(status1.best());
    CHECK(best.ind == best1.ind);
    CHECK(best.fit <= best1.fit);
    CHECK(best.fit >= best1.fit);
  }
}

TEST_CASE_FIXTURE(fixture1, "Concurrency")
{
  using namespace ultra;

  evolution_status<gp::individual, int> status;
  const gp::individual dummy(prob);

  constexpr unsigned MAX(1000);

  const auto work([&status, &dummy](bool odd)
  {
    for (unsigned i(1); i <= MAX; ++i)
    {
      if (i % 2 == odd)
        status.mutations = i;

      ++status.crossovers;

      if (odd)
      {
        if (i < 9 * MAX / 10 && i % 2 == odd)
          status.update_if_better({dummy, i});
      }
      else  // even
      {
        if (i % 100)
          status.update_if_better({dummy, i});
      }
    }
  });

  {
    std::jthread t1(work, false);
    std::jthread t2(work, true);
  }

  CHECK(status.mutations == MAX);
  CHECK(status.crossovers == 2*MAX);
  CHECK(status.best().fit == MAX - 1);
}

}  // TEST_SUITE("EVOLUTION STATUS")
