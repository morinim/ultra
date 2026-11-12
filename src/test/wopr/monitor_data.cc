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

#include "wopr/monitor_data.h"

#include "kernel/exceptions.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "wopr/monitor_data.cc"

TEST_SUITE("MONITOR DATA")
{

TEST_CASE("layers_line parsing")
{
  using namespace ultra::wopr;

  SUBCASE("empty line signals new run")
  {
    const layers_line ll("");
    CHECK(ll.new_run);
    CHECK(ll.empty());
    CHECK(ll.size() == 0);
  }

  SUBCASE("valid single layer")
  {
    // generation age_sup age_mean age_std_dev age_min age_max fit_mean fit_std_dev fit_min fit_max individuals
    const layers_line ll("10 5 2.5 1.0 1 4 0.95 0.05 0.8 1.0 100");
    CHECK(!ll.new_run);
    CHECK(ll.generation == 10);
    CHECK(ll.size() == 1);
    CHECK(!ll.empty());
    CHECK(ll.age_sup[0] == 5);
    CHECK(ll.age_mean[0] == doctest::Approx(2.5));
    CHECK(ll.age_std_dev[0] == doctest::Approx(1.0));
    CHECK(ll.age_min[0] == 1);
    CHECK(ll.age_max[0] == 4);
    CHECK(ll.fit_mean[0] == doctest::Approx(0.95));
    CHECK(ll.fit_std_dev[0] == doctest::Approx(0.05));
    CHECK(ll.fit_min[0] == doctest::Approx(0.8));
    CHECK(ll.fit_max[0] == doctest::Approx(1.0));
    CHECK(ll.individuals[0] == 100);
  }

  SUBCASE("valid multiple layers")
  {
    const layers_line ll(
      "0 0 1.0 0.5 1 2 0.5 0.1 0.3 0.7 50 10 2.0 0.8 1 3 0.8 0.2 0.6 0.9 60");
    CHECK(!ll.new_run);
    CHECK(ll.generation == 0);
    CHECK(ll.size() == 2);
    CHECK(ll.age_sup[0] == 0);
    CHECK(ll.individuals[0] == 50);
    CHECK(ll.age_sup[1] == 10);
    CHECK(ll.individuals[1] == 60);
  }

  SUBCASE("missing generation throws")
  {
    CHECK_THROWS_AS(layers_line("not_a_number"), ultra::exception::data_format);
  }

  SUBCASE("incomplete layer data throws")
  {
    // only 5 of 10 fields provided for layer
    CHECK_THROWS_AS(layers_line("1 5 2.5 1.0 1"), ultra::exception::data_format);
  }
}

TEST_CASE("population_line parsing")
{
  using namespace ultra::wopr;

  SUBCASE("empty line signals new run")
  {
    const population_line pl("");
    CHECK(pl.new_run);
    CHECK(pl.fit.empty());
  }

  SUBCASE("valid population line")
  {
    const population_line pl("5 0.1 10 0.2 20 0.3 30");
    CHECK(!pl.new_run);
    CHECK(pl.generation == 5);
    CHECK(pl.fit.size() == 3);
    CHECK(pl.fit[0] == doctest::Approx(0.1));
    CHECK(pl.obs[0] == doctest::Approx(10.0));
    CHECK(pl.fit[2] == doctest::Approx(0.3));
    CHECK(pl.obs[2] == doctest::Approx(30.0));
  }

  SUBCASE("incomplete observations throws")
  {
    CHECK_THROWS_AS(population_line("5 0.1"), ultra::exception::data_format);
  }
}

TEST_CASE("dynamic_data parsing")
{
  using namespace ultra::wopr;

  SUBCASE("empty line signals new run")
  {
    const dynamic_data dd("");
    CHECK(dd.new_run);
  }

  SUBCASE("valid dynamic data line")
  {
    const dynamic_data dd(
      "1 0.9 0.5 0.1 0.2 10 2.1 25 {1: 5, 2: 10} (+ x y)");
    CHECK(!dd.new_run);
    CHECK(dd.generation == 1);
    CHECK(dd.fit_best[0] == doctest::Approx(0.9));
    CHECK(dd.len_mean == 10);
    CHECK(dd.best_prg == "(+ x y)");
    CHECK(dd.crossover_types.size() == 2);
    CHECK(dd.crossover_types.at(1) == 5);
    CHECK(dd.crossover_types.at(2) == 10);
  }
}

}  // TEST_SUITE("MONITOR DATA")
