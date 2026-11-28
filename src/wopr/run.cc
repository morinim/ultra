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

#include "html_report.h"

#include "kernel/individual_format.h"
#include "kernel/search_log.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"
#include "utility/misc.h"
#include "utility/thread_pool.h"

#include "tinyxml2/tinyxml2.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

[[nodiscard]] summary_data make_summary_data(
  const ultra::search_stats<ultra::gp::individual, double> &stats,
  const ultra::model_measurements<double> &threshold)
{
  summary_data ret;
  if (!stats.runs())
    return ret;

  ret.runs = static_cast<unsigned>(stats.runs());
  ret.elapsed_time = stats.elapsed;
  ret.success_rate = stats.success_rate(threshold);

  if (!stats.fitness_distribution.empty())
  {
    ret.fit_mean = {stats.fitness_distribution.mean()};
    ret.fit_std_dev = {stats.fitness_distribution.standard_deviation()};
  }

  if (const auto &best(stats.best_measurements()); best.fitness)
    ret.best_fit = {*best.fitness};

  std::ostringstream ss;
  ss << ultra::out::in_line << stats.best_individual();
  ret.best_prg = ss.str();

  constexpr double perc(0.05);
  for (const auto &e : stats.elite_runs(perc))
  {
    ultra::model_measurements<ultra::fitnd> mm;
    if (e.best_measurements.fitness)
      mm.fitness = ultra::fitnd{*e.best_measurements.fitness};
    mm.accuracy = e.best_measurements.accuracy;
    ret.elite.emplace_back(static_cast<unsigned>(e.run), std::move(mm));
  }

  return ret;
}

struct live_report_entry
{
  rs::entry test {};
  fs::file_time_type last_write_time {};
};

[[nodiscard]] std::vector<live_report_entry> make_live_report_entries(
  const rs::collection_t &collection)
{
  std::vector<live_report_entry> entries;
  entries.reserve(collection.size());

  for (const auto &test : collection)
  {
    std::error_code ec;
    const auto mtime(fs::last_write_time(test.xml_summary, ec));
    entries.push_back({
      .test = test,
      .last_write_time = ec ? fs::file_time_type{} : mtime
    });
  }

  return entries;
}

void refresh_summaries(std::vector<live_report_entry> &entries)
{
  for (auto &entry : entries)
  {
    std::error_code ec;
    const auto mtime(fs::last_write_time(entry.test.xml_summary, ec));

    if (!ec && mtime != entry.last_write_time)
    {
      if (tinyxml2::XMLDocument summary;
          summary_data::check(entry.test.xml_summary, summary))
      {
        entry.test.current = summary_data(summary);
        entry.last_write_time = mtime;
      }
    }
  }
}

void run_live_report_worker(std::stop_token stoken,
                            std::vector<live_report_entry> entries,
                            fs::path html_path,
                            unsigned interval)
{
  std::condition_variable_any wake;
  std::mutex wake_mutex;

  const auto write_live([&]
  {
    refresh_summaries(entries);
    rs::collection_t snapshot_entries;
    snapshot_entries.reserve(entries.size());
    for (const auto &entry : entries)
      snapshot_entries.push_back(entry.test);
    const results_snapshot snapshot(snapshot_entries, rs::exec_mode::run);
    (void)write_html_report(html_path, snapshot, true, interval);
  });

  write_live();

  while (!stoken.stop_requested())
  {
    std::unique_lock lock(wake_mutex);
    wake.wait_for(lock, stoken, std::chrono::seconds(interval),
                  [] { return false; });
    if (stoken.stop_requested())
      break;

    write_live();
  }
}

[[nodiscard]] summary_data run_dataset_search(const rs::entry &test,
                                              const std::stop_source &source,
                                              bool multiple_tests)
{
  if (source.stop_requested())
  {
    test.activity->set(rs::task_state::cancelled);
    return {};
  }

  test.activity->set(rs::task_state::running);
  try
  {
    const auto &dataset(test.dataset);
    ultra::src::problem prob(
      ultra::src::dataframe(dataset, test.conf.params),
      ultra::symbol_init::all);
    prob.params.evolution.generations = test.conf.generations;
    prob.params.common_random_numbers = test.conf.common_random_numbers;

    ultra::src::search s(prob);
    auto sl(rs::make_search_log(dataset, test.xml_summary));
    s.logger(sl).stop_source(source);

    if (multiple_tests)
      s.tag(dataset.stem().string());

    const auto stats(s.run(test.conf.runs, test.conf.threshold));
    test.activity->set(source.stop_requested() ? rs::task_state::cancelled
                                               : rs::task_state::finished);
    return make_summary_data(stats, test.conf.threshold);
  }
  catch (const std::exception &e)
  {
    test.activity->set(rs::task_state::failed, e.what());
    throw;
  }
  catch (...)
  {
    test.activity->set(rs::task_state::failed, "Unknown error");
    throw;
  }
}

}  // namespace

bool rs::run::start(options options)
{
  for (auto &test : options.collection)
    if (!test.activity)
      test.activity = std::make_shared<dataset_activity>();

  std::stop_source source;
  const bool multiple_tests(options.collection.size() > 1);

  if (multiple_tests)
    ultra::log::reporting_level = ultra::log::lPAROUT;

  const auto thread_limit(options.threads ? options.threads
                                          : hardware_threads());
  ultra::thread_pool pool(std::min(thread_limit, options.collection.size()));

  // Cancels running searches if task launch or live reporting throws,
  // preventing the thread pool from hanging during shutdown.
  struct stop_on_exit
  {
    std::stop_source &source_ref;
    ~stop_on_exit() { source_ref.request_stop(); }
  } guard{source};

  std::jthread html_updater;
  if (!options.html_output.empty() && options.interval > 0)
  {
    auto entries(make_live_report_entries(options.collection));
    std::cout << "HTML dashboard: " << options.html_output
              << " (refreshing every " << options.interval << "s)\n";
    html_updater = std::jthread(run_live_report_worker, std::move(entries),
                                options.html_output, options.interval);
  }

  std::vector<std::future<summary_data>> tasks;
  tasks.reserve(options.collection.size());

  for (const auto &test : options.collection)
    tasks.emplace_back(pool.submit(run_dataset_search, test, source,
                                   multiple_tests));

  pool.wait();

  if (html_updater.joinable())
  {
    html_updater.request_stop();
    html_updater.join();
  }

  bool successful(true);
  for (std::size_t i(0); i < tasks.size(); ++i)
  {
    auto &test(options.collection[i]);
    try
    {
      test.current = tasks[i].get();
    }
    catch (const std::exception &e)
    {
      std::cerr << "Search of `" << test.dataset << "` failed: " << e.what()
                << '\n';
      successful = false;
    }
    catch (...)
    {
      std::cerr << "Search of `" << test.dataset
                << "` failed: unknown error.\n";
      successful = false;
    }
  }

  if (!options.html_output.empty())
  {
    const results_snapshot snapshot(options.collection, exec_mode::run);
    if (!write_html_report(options.html_output, snapshot))
      successful = false;
  }

  return successful;
}

}  // namespace ultra::wopr
