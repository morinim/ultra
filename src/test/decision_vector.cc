/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/decision_vector.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("decision_vector")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  static_assert(DecisionVectorExtractable<gp::individual>);
  static_assert(NumericalOptimisable<gp::individual>);

  static_assert(DecisionVectorExtractable<gp::team<gp::individual>>);
  static_assert(NumericalOptimisable<gp::team<gp::individual>>);

  struct without_dv {};
  static_assert(!DecisionVectorExtractable<without_dv>);
  static_assert(!NumericalOptimisable<without_dv>);
}

}  // TEST_SUITE
