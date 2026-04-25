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

#include "test/fixture1.h"

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


TEST_CASE_FIXTURE(fixture1, "decision_vector round-trip")
{
  ultra::gp::individual ind(prob);

  const auto before(extract_decision_vector(ind));

  auto copy(ind);
  copy.apply_decision_vector(before);

  const auto after(extract_decision_vector(copy));

  CHECK(before.values == after.values);
  CHECK(before.coords.size() == after.coords.size());

  for (std::size_t i(0); i < before.coords.size(); ++i)
  {
    CHECK(before.coords[i].coord.loc == after.coords[i].coord.loc);
    CHECK(before.coords[i].coord.arg_index == after.coords[i].coord.arg_index);
    CHECK(before.coords[i].kind == after.coords[i].kind);
  }
}

}  // TEST_SUITE
