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

#include "kernel/alps.h"
#include "kernel/layered_population.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("ALPS")
{

TEST_CASE_FIXTURE(fixture1, "Set age")
{
  using namespace ultra;

  prob.env.population.init_layers = 10;
  prob.env.population.individuals = random::between(10, 20);
  layered_population<gp::individual> pop(prob);

  alps::set_age(pop);

  for (std::size_t l(1); l < pop.layers(); ++l)
  {
    CHECK(pop.layer(l).max_age() > pop.layer(l - 1).max_age());
    CHECK(pop.layer(l - 1).max_age() == prob.env.alps.max_age(l - 1));
  }

  using age_t = decltype(pop.back().max_age());
  CHECK(pop.back().max_age() == std::numeric_limits<age_t>::max());
}

}  // TEST_SUITE("ALPS")
