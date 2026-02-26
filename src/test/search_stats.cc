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

#include "kernel/search_stats.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <chrono>
#include <cmath>
#include <limits>

namespace ultra::test
{

using fitness_t = double;
using individual_t = ultra::gp::individual;
using measurements_t = ultra::model_measurements<fitness_t>;

[[nodiscard]] measurements_t mm(std::optional<fitness_t> fitness,
                                std::optional<double> accuracy = std::nullopt)
{
  measurements_t m{};
  m.fitness = fitness;
  m.accuracy = accuracy;
  return m;
}

}  // namespace ultra::test


TEST_SUITE("search_stats")
{

using namespace ultra::test;
using namespace std::chrono_literals;

TEST_CASE("empty stats: success_rate is zero")
{
  ultra::search_stats<individual_t, fitness_t> s;
  CHECK(s.runs() == 0);
  CHECK(s.success_rate() == doctest::Approx(0.0));
  CHECK(s.good_runs.empty());
  CHECK(s.elapsed == 0ms);
}

TEST_CASE_FIXTURE(fixture1,
                  "update increments runs and accumulates elapsed time")
{
  ultra::search_stats<individual_t, fitness_t> s;

  const auto ind1 = ultra::gp::individual(prob);
  const auto ind2 = ultra::gp::individual(prob);

  s.update(ind1, mm(10.0), 120ms, {});
  s.update(ind2, mm(9.0), 30ms,  {});

  CHECK(s.runs() == 2);
  CHECK(s.elapsed == 150ms);
}

TEST_CASE_FIXTURE(fixture1,
                  "best run is the one with best measurements ordering")
{
  ultra::search_stats<individual_t, fitness_t> s;

  s.update(ultra::gp::individual(prob), mm(1.0), 1ms, {});
  s.update(ultra::gp::individual(prob), mm(3.0), 1ms, {});
  s.update(ultra::gp::individual(prob), mm(2.0), 1ms, {});

  CHECK(s.runs() == 3);
  CHECK(s.best_measurements().fitness.has_value());
  CHECK(*s.best_measurements().fitness == doctest::Approx(3.0));
  CHECK(s.best_run() == 1);
}

TEST_CASE_FIXTURE(
  fixture1,
  "good runs are recorded only when threshold specifies a criterion")
{
  ultra::search_stats<individual_t, fitness_t> s{};

  // Threshold with no criteria: should not tag any run as good.
  s.update(ultra::gp::individual(prob), mm(100.0), 1ms, {});
  s.update(ultra::gp::individual(prob), mm(0.0),   1ms, {});

  CHECK(s.runs() == 2);
  CHECK(s.good_runs.empty());
  CHECK(s.success_rate() == doctest::Approx(0.0));

  // Now a real threshold: should start tagging good runs.
  const auto thr(mm(50.0, std::nullopt));

  s.update(ultra::gp::individual(prob), mm(49.0), 1ms, thr);  // not good
  s.update(ultra::gp::individual(prob), mm(50.0), 1ms, thr);  // good
  s.update(ultra::gp::individual(prob), mm(60.0), 1ms, thr);  // good

  CHECK(s.runs() == 5);
  CHECK(s.good_runs.size() == 2);
  CHECK(s.good_runs.contains(3));
  CHECK(s.good_runs.contains(4));
  CHECK(s.success_rate() == doctest::Approx(2.0 / 5.0));
}

}  // TEST_SUITE
