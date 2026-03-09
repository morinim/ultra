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

#include "kernel/evolution_selection.h"
#include "kernel/distribution.h"
#include "kernel/layered_population.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/debug_support.h"
#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("EVOLUTION SELECTION")
{

TEST_CASE_FIXTURE(fixture1, "Tournament")
{
  using namespace ultra;

  prob.params.population.individuals    = 20;
  prob.params.population.init_subgroups =  1;

  // The test assumes independent draws.
  prob.params.evolution.mate_zone = std::numeric_limits<std::size_t>::max();

  // Individuals have distinct ages.
  const auto pop(debug::make_debug_population<gp::individual>(prob));

  test_evaluator<gp::individual> eva(test_evaluator_type::age);

  selection::tournament select(eva, prob.params);

  // Every individual has a unique fitness (`make_debug_population`),
  // so there is just one maximum-fitness-individual.
  for (unsigned ts(1); ts < prob.params.population.individuals; ++ts)
  {
    prob.params.evolution.tournament_size = ts;

    auto max(std::ranges::max(pop, [&eva](const auto &p1, const auto &p2)
                                   {
                                     return eva(p1) < eva(p2);
                                   }));

    const auto n(prob.params.population.individuals * 100);
    unsigned found(0);
    for (unsigned i(0); i < n; ++i)
    {
      auto parents(select(pop.front()));

      CHECK(parents.size() == prob.params.evolution.tournament_size);

      const bool is_sorted(
        std::ranges::is_sorted(parents,
                               [&](const auto &p1, const auto &p2)
                               {
                                 return eva(p1) > eva(p2);
                               }));
      CHECK(is_sorted);

      if (std::ranges::find_if(parents,
                               [ma = max.age()](const auto &prg)
                               {
                                 return prg.age() == ma;
                               })
          != parents.end())
        ++found;
    }

    const double frequency(static_cast<double>(found) / n);
    const double p_not_present((pop.size() - 1)
                               / static_cast<double>(pop.size()));
    const double p_present(1.0 - std::pow(p_not_present, ts));
    CHECK(frequency > p_present - 0.1);
    CHECK(frequency < p_present + 0.1);
  }
}

// Verify the effect of `alps.p_main_layer` in ALPS tournament selection.
//
// The test constructs two clearly distinguishable layers:
// - the primary layer (layer 1) contains only weak individuals;
// - the secondary layer (layer 0) contains only strong individuals.
// All individuals are young, so ALPS comparison reduces to evaluator fitness.
//
// Because secondary individuals strictly dominate primary ones, any sampled
// secondary candidate will replace a primary parent in the tournament result.
// Given the implementation:
// - the first two candidates are always sampled from the primary layer;
// - the remaining `tournament_size - 1` candidates are sampled from primary
//   with probability `p_main_layer` and secondary otherwise.
//
// The probability that at least one selected parent comes from the secondary
// layer is `1 - p_main_layer^(tournament_size - 1)`, while the probability
// that both selected parents come from the secondary layer is
// `(1 - p_main_layer)^(tournament_size - 1)` when enough rounds exist.
//
// The test measures these frequencies empirically and checks them against
// the expected values within a statistical tolerance.
TEST_CASE_FIXTURE(fixture1, "ALPS p_main_layer")
{
  using namespace ultra;

  struct selection_stats
  {
    double any_secondary {};
    double both_secondary {};
  };

  const auto run =
    [this](double p_main_layer, unsigned tournament) -> selection_stats
    {
      prob.params.population.individuals    = 50;
      prob.params.population.init_subgroups = 2;
      prob.params.evolution.tournament_size = tournament;
      prob.params.alps.p_main_layer         = p_main_layer;

      layered_population<gp::individual> pop(prob);
      test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

      REQUIRE(pop.layers() == 2);
      REQUIRE(!pop.layer(0).empty());
      REQUIRE(!pop.layer(1).empty());

      // Build two prototypes with different signatures / evaluator values.
      // Layer 1 (primary) will contain the weaker individual.
      // Layer 0 (secondary) will contain the stronger individual.
      gp::individual weak(prob);
      gp::individual strong(prob);

      while (almost_equal(eva(strong), eva(weak)))
        strong = gp::individual(prob);

      if (eva(strong) < eva(weak))
        std::swap(strong, weak);

      const auto weak_sig(weak.signature());
      const auto strong_sig(strong.signature());

      // Make every individual young. Since age defaults to 0, assigning these
      // prototypes is enough provided they have not been aged elsewhere.
      REQUIRE(weak.age() == 0);
      REQUIRE(strong.age() == 0);

      for (auto &prg : pop.layer(1))
        prg = weak;

      for (auto &prg : pop.layer(0))
        prg = strong;

      const auto &cpop(pop);

      const auto source_layer(
        [&](const gp::individual &prg) -> std::size_t
        {
          const auto sig(prg.signature());

          if (sig == weak_sig)
            return 1;  // primary layer
          if (sig == strong_sig)
            return 0;  // secondary layer

          FAIL("Selected individual does not match any test prototype");
          return 0;
        });

      unsigned any_secondary(0);
      unsigned both_secondary(0);

      constexpr unsigned n(10000);

      for (unsigned i(0); i < n; ++i)
      {
        selection::alps select(eva, prob.params);

        const auto ln(std::next(cpop.range_of_layers().begin()));
        const auto parents(select(alps::selection_layers(cpop, ln)));

        CHECK(parents.size() == 2);

        const auto l0(source_layer(parents[0]));
        const auto l1(source_layer(parents[1]));

        if (l0 == 0 || l1 == 0)
          ++any_secondary;

        if (l0 == 0 && l1 == 0)
          ++both_secondary;
      }

      return {any_secondary / double(n), both_secondary / double(n)};
    };

  constexpr double tolerance(0.03);

  SUBCASE("Tournament 1")
  {
    for (const double p : {0.0, 0.5, 1.0})
    {
      const auto res(run(p, 1));

      CHECK(res.any_secondary == doctest::Approx(0.0));
      CHECK(res.both_secondary == doctest::Approx(0.0));
    }
  }

  SUBCASE("Tournament 2")
  {
    for (const double p : {0.0, 0.25, 0.5, 0.75, 1.0})
    {
      const auto res(run(p, 2));

      const double expected_any(1.0 - p);

      CHECK(expected_any - tolerance <= res.any_secondary);
      CHECK(res.any_secondary <= expected_any + tolerance);

      // With tournament size 2 there is only one extra sampled candidate,
      // therefore both selected parents cannot both come from secondary.
      CHECK(res.both_secondary == doctest::Approx(0.0));
    }
  }

  SUBCASE("Tournament 3")
  {
    for (const double p : {0.0, 0.25, 0.5, 0.75, 1.0})
    {
      const auto res(run(p, 3));

      const double expected_any(1.0 - p * p);
      const double expected_both((1.0 - p) * (1.0 - p));

      CHECK(expected_any - tolerance <= res.any_secondary);
      CHECK(res.any_secondary <= expected_any + tolerance);

      CHECK(expected_both - tolerance <= res.both_secondary);
      CHECK(res.both_secondary <= expected_both + tolerance);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "ALPS Concurrency")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;
  prob.params.evolution.tournament_size = 10;
  prob.params.alps.p_main_layer         = .5;

  layered_population<gp::individual> pop(prob);
  test_evaluator<gp::individual> eva(test_evaluator_type::fixed);
  selection::alps select(eva, prob.params);

  const auto search([&](auto from_layers)
  {
    for (unsigned i(0); i < 5000; ++i)
    {
      const auto parents(select(from_layers));

      for (const auto &p : parents)
        CHECK(p.is_valid());
    }
  });

  {
    std::vector<std::jthread> threads;

    const auto range(pop.range_of_layers());
    for (auto l(range.begin()); l != range.end(); ++l)
      threads.emplace_back(search, alps::selection_layers(pop, l));
  }
}

TEST_CASE_FIXTURE(fixture4, "DE")
{
  using namespace ultra;

  prob.params.population.individuals    = 100;
  prob.params.population.init_subgroups =   1;

  // The test assumes independent draws.
  prob.params.evolution.mate_zone = std::numeric_limits<std::size_t>::max();

  distribution<double> dist;

  // Individuals have distinct ages.
  auto pop(debug::make_debug_population<de::individual>(prob));

  test_evaluator<de::individual> eva(test_evaluator_type::realistic);

  selection::de select;

  auto max_age(debug::best_individual(pop, eva).age());

  const unsigned n(prob.params.population.individuals * 100);
  unsigned found(0);
  for (unsigned i(0); i < n; ++i)
  {
    auto selected(select(pop.front()));

    if (selected.target.age() == max_age || selected.base.age() == max_age
        || selected.a.age() == max_age || selected.b.age() == max_age)
      ++found;

    dist.add(selected.target.age());
    dist.add(selected.base.age());
    dist.add(selected.a.age());
    dist.add(selected.b.age());
  }

  const double frequency(static_cast<double>(found) / n);
  const double p_not_present((pop.size() - 1)
                             / static_cast<double>(pop.size()));
  const double p_present(1.0 - std::pow(p_not_present, 4));
  CHECK(frequency > p_present - 0.1);
  CHECK(frequency < p_present + 0.1);

  const double avg(pop.size() / 2);
  const double delta(pop.size() / 20);
  CHECK(avg - delta <= dist.mean());
  CHECK(dist.mean() <= avg + delta);
}

}  // TEST_SUITE("EVOLUTION SELECTION")
