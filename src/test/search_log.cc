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

#include "kernel/search_log.h"
#include "kernel/evolution.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SEARCH LOG")
{

TEST_CASE("Filenames")
{
  using namespace ultra;

  SUBCASE("Default filenames")
  {
    CHECK(std::filesystem::path(search_log::default_dynamic_file).extension()
          == ".txt");
    CHECK(std::filesystem::path(search_log::default_layers_file).extension()
          == ".txt");
    CHECK(std::filesystem::path(search_log::default_population_file).extension()
          == ".txt");
    CHECK(std::filesystem::path(search_log::default_summary_file).extension()
          == ".xml");
  }

  SUBCASE("Basename")
  {
    const std::string basename("test.csv");
    const std::string stem("test");

    const auto dyn(dynamic_from_basename(basename));
    CHECK(dyn.string().find(stem) == 0);
    CHECK(dyn.string().find(basename) == std::string::npos);
    CHECK(dyn.string().find(search_log::default_dynamic_file)
          != std::string::npos);
    CHECK(dyn.extension() == ".txt");

    const auto lys(layers_from_basename(basename));
    CHECK(lys.string().find(stem) == 0);
    CHECK(lys.string().find(basename) == std::string::npos);
    CHECK(lys.string().find(search_log::default_layers_file)
          != std::string::npos);
    CHECK(lys.extension() == ".txt");

    const auto pop(population_from_basename(basename));
    CHECK(pop.string().find(stem) == 0);
    CHECK(pop.string().find(basename) == std::string::npos);
    CHECK(pop.string().find(search_log::default_population_file)
          != std::string::npos);
    CHECK(pop.extension() == ".txt");

    const auto sum(summary_from_basename(basename));
    CHECK(sum.string().find(stem) == 0);
    CHECK(sum.string().find(basename) == std::string::npos);
    CHECK(sum.string().find(search_log::default_summary_file)
          != std::string::npos);
    CHECK(sum.extension() == ".xml");

    CHECK(summary_from_basename("/path/to/file.csv")
          == "/path/to/file.summary.xml");

    CHECK(basename_from_summary(sum) == basename);
    CHECK(basename_from_summary("/path/to/file.summary.xml")
          == "/path/to/file.csv");
  }
}

TEST_CASE_FIXTURE(fixture1, "Saving snapshots")
{
  using namespace ultra;

  prob.params.population.individuals    = 30;
  prob.params.population.init_subgroups =  4;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  evolution evo(prob, eva);

  SUBCASE("Default - No log")
  {
    const auto sum(evo.run<alps_es>());

    CHECK(!sum.best().empty());
    CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));

    CHECK(!std::filesystem::exists(search_log::default_dynamic_file));
    CHECK(!std::filesystem::exists(search_log::default_layers_file));
    CHECK(!std::filesystem::exists(search_log::default_population_file));
  }

  SUBCASE("Default - User specified logs")
  {
    search_log logger;
    logger.dynamic_file_path = search_log::default_dynamic_file;
    logger.layers_file_path = search_log::default_layers_file;
    logger.population_file_path = search_log::default_population_file;

    const auto sum(evo.logger(logger).run<alps_es>());

    CHECK(!sum.best().empty());
    CHECK(eva(sum.best().ind) == doctest::Approx(sum.best().fit));

    CHECK(std::filesystem::exists(search_log::default_dynamic_file));
    CHECK(std::filesystem::exists(search_log::default_layers_file));
    CHECK(std::filesystem::exists(search_log::default_population_file));
  }

  std::filesystem::remove(search_log::default_dynamic_file);
  std::filesystem::remove(search_log::default_layers_file);
  std::filesystem::remove(search_log::default_population_file);
}

TEST_CASE_FIXTURE(fixture1, "Saving summary")
{
  using namespace ultra;

  std::filesystem::remove(search_log::default_summary_file);

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);

  search_log logger;
  logger.summary_file_path = search_log::default_summary_file;

  search_stats<gp::individual, double> stats;
  stats.best_individual = gp::individual(prob);
  stats.best_measurements.fitness = eva(stats.best_individual);
  stats.fitness_distribution.add(*stats.best_measurements.fitness);
  stats.good_runs.insert(0);
  stats.best_run = 0;
  stats.runs = 1;
  logger.save_summary(stats);

  CHECK(std::filesystem::exists(search_log::default_summary_file));
}

}  // TEST_SUITE("SEARCH LOG")
