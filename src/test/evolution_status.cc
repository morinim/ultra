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

#include "kernel/evolution_status.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION STATUS")
{

TEST_CASE_FIXTURE(fixture1, "update_if_better")
{
  using namespace ultra;

  unsigned generation(0);
  evolution_status<gp::individual, int> status(&generation);

  CHECK(status.best().empty());
  CHECK(status.generation() == generation);
  CHECK(status.last_improvement() == 0);

  const gp::individual prg(prob);

  generation = 10;
  status.update_if_better(scored_individual{prg, 10});

  CHECK(status.best().ind == prg);
  CHECK(status.generation() == generation);
  CHECK(status.last_improvement() == generation);
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Missing best")
  {
    evolution_status<gp::individual, fitnd> status;

    CHECK(status.best().empty());

    CHECK(status.save(ss));

    decltype(status) status1;
    CHECK(status1.load(ss, prob));

    CHECK(status.last_improvement()  == status1.last_improvement());
    CHECK(status1.best().empty());
  }

  SUBCASE("With best")
  {
    const unsigned generation(10);
    evolution_status<gp::individual, fitnd> status(&generation);
    status.update_if_better(scored_individual(gp::individual(prob),
                                              fitnd{1.0, 2.0}));

    CHECK(status.last_improvement() == generation);

    CHECK(!status.best().empty());

    CHECK(status.save(ss));

    decltype(status) status1;
    CHECK(status1.load(ss, prob));

    CHECK(status1.last_improvement() == generation);

    const auto best(status.best()), best1(status1.best());
    CHECK(best.ind == best1.ind);
    CHECK(best.fit <= best1.fit);
    CHECK(best.fit >= best1.fit);
  }
}

}  // TEST_SUITE("EVOLUTION STATUS")
