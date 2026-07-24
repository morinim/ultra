/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "results.h"

#include "command_line.h"

#include "kernel/search_log.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"

#include <filesystem>
#include <future>
#include <iostream>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

bool rs::run::start(const imgui_app::program::settings &settings,
                    options options)
{
  std::stop_source source;
  const bool multiple_tests(options.collection.size() > 1);

  const auto test_driver(
    [source, multiple_tests](auto test)
    {
      const auto &dataset(test.second.dataset);
      ultra::src::problem prob(
        ultra::src::dataframe(dataset, test.second.conf.params),
        ultra::symbol_init::all);
      prob.params.evolution.generations = test.second.conf.generations;

      ultra::src::search s(prob);

      ultra::search_log sl;
      const fs::path base_dir(dataset.parent_path());

      sl.dynamic_file_path = build_path(
        base_dir, ultra::dynamic_from_basename(dataset));
      sl.layers_file_path = build_path(
        base_dir, ultra::layers_from_basename(dataset));
      sl.population_file_path = build_path(
        base_dir, ultra::population_from_basename(dataset));
      sl.summary_file_path = test.second.xml_summary;

      s.logger(sl).stop_source(source);

      if (multiple_tests)
        s.tag(dataset.stem());

      return s.run(test.second.conf.runs, test.second.conf.threshold);
    });

  if (multiple_tests)
    ultra::log::reporting_level = ultra::log::lPAROUT;

  using stats_t = ultra::search_stats<ultra::gp::individual, double>;
  using task_t = std::pair<fs::path, std::future<stats_t>>;

  std::vector<task_t> tasks;

  // If task launch or the GUI throws, cancels running searches before stack
  // unwinding destroys their futures and waits for them to finish.
  struct stop_on_exit
  {
    std::stop_source &source_ref;
    ~stop_on_exit() { source_ref.request_stop(); }
  } guard{source};

  for (const auto &test : options.collection)
    tasks.emplace_back(test.second.dataset,
                       std::async(std::launch::async, test_driver, test));

  if (!options.nogui)
  {
    rs::summary::start(
      settings, rs::summary::options{std::move(options.collection)});
    source.request_stop();
  }

  bool successful(true);
  for (auto &[dataset, result] : tasks)
  {
    try
    {
      std::ignore = result.get();
    }
    catch (const std::exception &e)
    {
      std::cerr << "Search of `" << dataset << "` failed: " << e.what() << '\n';
      successful = false;
    }
    catch (...)
    {
      std::cerr << "Search of `" << dataset << "` failed: unknown error.\n";
      successful = false;
    }
  }

  return successful;
}

}  // namespace ultra::wopr
