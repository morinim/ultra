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
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("ANALYZER")
{

TEST_CASE_FIXTURE(fixture1, "Base GP")
{
  using namespace ultra;
  using IND = gp::individual;

  constexpr double sup_fit(10.0);
  const auto eva([](const IND &) { return random::sup(sup_fit); });

  prob.params.population.min_individuals = 1;

  const auto sum_crossovers([](const auto &cts)
  {
    namespace rv = std::ranges::views;

    const auto v(cts | rv::values | rv::common);
    return std::accumulate(v.begin(), v.end(), 0);
  });

  for (std::size_t l(1); l < 3; ++l)
    for (std::size_t i(1); i < 50; ++i)
      for (std::size_t c(10); c < 20; ++c)
      {
        prob.params.population.init_subgroups = l;
        prob.params.population.individuals = i;
        prob.params.slp.code_length = c;

        layered_population<IND> pop(prob);

        analyzer<IND, double> az;
        for (auto it(pop.begin()), end(pop.end()); it != end; ++it)
          az.add(*it, eva(*it) / l, it.uid());

        CHECK(az.is_valid());

        const auto n(l * i);
        CHECK(az.age_dist().size() == n);
        CHECK(az.fit_dist().size() == n);
        CHECK(az.length_dist().size() == n);

        CHECK(sum_crossovers(az.crossover_types()) == n);

        if (n > 20)
          CHECK(az.crossover_types().size() > 1);

        for (const auto range(pop.range_of_layers());
             const auto &layer : range)
        {
          CHECK(az.age_dist(layer).size() == i);
          CHECK(az.fit_dist(layer).size() == i);
          CHECK(az.length_dist(layer).size() == i);

          CHECK(sum_crossovers(az.crossover_types(layer)) == i);

          CHECK(az.age_dist(layer).min() >= az.age_dist().min());
          CHECK(az.age_dist(layer).max() <= az.age_dist().max());

          CHECK(az.fit_dist(layer).min() >= az.fit_dist().min());
          CHECK(az.fit_dist(layer).max() <= az.fit_dist().max());

          CHECK(az.length_dist(layer).min() >= az.length_dist().min());
          CHECK(az.length_dist(layer).max() <= az.length_dist().max());
        }

        CHECK(az.age_dist().min() == doctest::Approx(az.age_dist().max()));
        CHECK(0.0 <= az.fit_dist().min());
        CHECK(az.fit_dist().max() < sup_fit / l);
        CHECK(1 <= az.length_dist().min());
        CHECK(az.length_dist().max() <= c);
      }
}

TEST_CASE_FIXTURE(fixture4, "Base DE")
{
  using namespace ultra;
  using IND = de::individual;

  constexpr double sup_fit(10.0);
  const auto eva([](const IND &) { return random::sup(sup_fit); });

  prob.params.population.min_individuals = 1;

  for (std::size_t l(1); l < 3; ++l)
    for (std::size_t i(1); i < 50; ++i)
      for (std::size_t c(10); c < 20; ++c)
      {
        prob.params.population.init_subgroups = l;
        prob.params.population.individuals = i;

        layered_population<IND> pop(prob);

        analyzer<IND, double> az;
        for (auto it(pop.begin()), end(pop.end()); it != end; ++it)
          az.add(*it, eva(*it) / l, it.uid());

        CHECK(az.is_valid());

        const auto n(l * i);
        CHECK(az.age_dist().size() == n);
        CHECK(az.fit_dist().size() == n);
        CHECK(az.length_dist().size() == n);

        CHECK(az.crossover_types().empty());

        for (const auto range(pop.range_of_layers());
             const auto &layer : range)
        {
          CHECK(az.age_dist(layer).size() == i);
          CHECK(az.fit_dist(layer).size() == i);
          CHECK(az.length_dist(layer).size() == i);

          CHECK(az.crossover_types(layer).empty());

          CHECK(az.age_dist(layer).min() >= az.age_dist().min());
          CHECK(az.age_dist(layer).max() <= az.age_dist().max());

          CHECK(az.fit_dist(layer).min() >= az.fit_dist().min());
          CHECK(az.fit_dist(layer).max() <= az.fit_dist().max());

          CHECK(static_cast<std::size_t>(az.length_dist(layer).min())
                == prob.parameters());
          CHECK(static_cast<std::size_t>(az.length_dist(layer).max())
                == prob.parameters());
        }

        CHECK(az.age_dist().min() == doctest::Approx(az.age_dist().max()));
        CHECK(0.0 <= az.fit_dist().min());
        CHECK(az.fit_dist().max() < sup_fit / l);
        CHECK(static_cast<std::size_t>(az.length_dist().min())
              == prob.parameters());
        CHECK(static_cast<std::size_t>(az.length_dist().max())
              == prob.parameters());
      }
}

}  // TEST_SUITE("ANALYZER")
