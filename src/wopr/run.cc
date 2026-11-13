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

#include "kernel/search_log.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"
#include "utility/misc.h"
#include "utility/thread_pool.h"

#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <stop_token>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

bool run_impl(
  rs::run::options options,
  const std::function<void(std::stop_source &)> &on_started = {})
{
  std::stop_source source;
  const bool multiple_tests(options.collection.size() > 1);

  const auto test_driver(
    [source, multiple_tests](const auto &test)
    {
      if (source.stop_requested())
        return;

      const auto &dataset(test.dataset);
      ultra::src::problem prob(
        ultra::src::dataframe(dataset, test.conf.params),
        ultra::symbol_init::all);
      prob.params.evolution.generations = test.conf.generations;

      ultra::src::search s(prob);
      auto sl(rs::make_search_log(dataset, test.xml_summary));
      s.logger(sl).stop_source(source);

      if (multiple_tests)
        s.tag(dataset.stem());

      s.run(test.conf.runs, test.conf.threshold);
    });

  if (multiple_tests)
    ultra::log::reporting_level = ultra::log::lPAROUT;

  const auto thread_limit(options.threads ? options.threads
                                          : hardware_threads());
  ultra::thread_pool pool(std::min(thread_limit, options.collection.size()));

  // Cancels running searches if task launch or the GUI throws, preventing
  // the thread pool from hanging during shutdown.
  struct stop_on_exit
  {
    std::stop_source &source_ref;
    ~stop_on_exit() { source_ref.request_stop(); }
  } guard{source};

  using task_t = std::pair<fs::path, std::future<void>>;
  std::vector<task_t> tasks;
  tasks.reserve(options.collection.size());

  for (const auto &test : options.collection)
    tasks.emplace_back(test.dataset,
                       pool.submit(test_driver, test));

  if (on_started)
    on_started(source);

  pool.wait();

  bool successful(true);
  for (auto &[dataset, result] : tasks)
  {
    try
    {
      result.get();
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

}  // namespace

bool rs::run::start(options options)
{
#if defined(ULTRA_WOPR_GUI)
  if (!options.nogui)
  {
    auto summary_collection(options.collection);
    return run_impl(
      std::move(options),
      [&summary_collection, demo = options.imgui_demo](std::stop_source &source)
      {
        rs::summary::start(
          rs::summary::options{std::move(summary_collection), demo,
                               rs::exec_mode::run});
        source.request_stop();
      });
  }
#endif

  return run_impl(std::move(options));
}

}  // namespace ultra::wopr
