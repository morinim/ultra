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

#include <numbers>

#include "kernel/analyzer.h"
#include "kernel/gp/individual.h"
#include "kernel/layered_population.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("ANALYZER")
{

TEST_CASE_FIXTURE(fixture1, "Base")
{
  using namespace ultra;

  constexpr double sup_fit(10.0);
  const auto eva([](const auto &) { return random::sup(sup_fit); });

  for (std::size_t l(1); l < 3; ++l)
    for (std::size_t i(1); i < 50; ++i)
      for (std::size_t c(10); c < 20; ++c)
      {
        prob.env.population.init_layers = l;
        prob.env.population.individuals = i;
        prob.env.slp.code_length = c;

        layered_population<gp::individual> pop(prob);

        analyzer<gp::individual, double> az;

        for (auto it(pop.begin()), end(pop.end()); it != end; ++it)
          az.add(*it, eva(*it) / l, it.layer());

        const auto n(l * i);
        CHECK(az.age_dist().size() == n);
        CHECK(az.fit_dist().size() == n);
        CHECK(az.length_dist().size() == n);

        for (std::size_t j(0); j < l; ++j)
        {
          CHECK(az.age_dist(j).size() == i);
          CHECK(az.fit_dist(j).size() == i);
          CHECK(az.length_dist(j).size() == i);

          CHECK(az.age_dist(j).min() >= az.age_dist().min());
          CHECK(az.age_dist(j).max() <= az.age_dist().max());

          CHECK(az.fit_dist(j).min() >= az.fit_dist().min());
          CHECK(az.fit_dist(j).max() <= az.fit_dist().max());

          CHECK(az.length_dist(j).min() >= az.length_dist().min());
          CHECK(az.length_dist(j).max() <= az.length_dist().max());
        }

        CHECK(az.age_dist().min() == doctest::Approx(az.age_dist().max()));
        CHECK(0.0 <= az.fit_dist().min());
        CHECK(az.fit_dist().max() < sup_fit / l);
        CHECK(1 <= az.length_dist().min());
        CHECK(az.length_dist().max() <= c);
      }
}

}  // TEST_SUITE("FUNCTION")
