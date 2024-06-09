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

TEST_SUITE("EVOLUTION SUMMARY 2")
{

TEST_CASE_FIXTURE(fixture1, "update_if_better")
{
  using namespace ultra;

  summary<gp::individual, fitnd> s;

  const scored_individual si1(gp::individual(prob), fitnd{1.0, 2.0});
  CHECK(s.update_if_better(si1));
}

}  // TEST_SUITE("EVOLUTION SUMMARY")
