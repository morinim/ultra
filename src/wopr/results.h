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

#if !defined(ULTRA_WOPR_RESULTS_H)
#define      ULTRA_WOPR_RESULTS_H

#include "kernel/fitness.h"
#include "kernel/model_measurements.h"
#include "kernel/search_log.h"
#include "kernel/gp/src/dataframe.h"

#include <chrono>
#include <expected>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tinyxml2 { class XMLDocument; }

namespace ultra::wopr
{

[[nodiscard]] std::filesystem::path build_path(
  std::filesystem::path, std::filesystem::path);

struct summary_data
{
  summary_data() = default;
  explicit summary_data(const tinyxml2::XMLDocument &);
  explicit summary_data(const std::filesystem::path &);

  [[nodiscard]] bool empty() const noexcept { return !runs; }

  [[nodiscard]] static bool check(const std::filesystem::path &,
                                  tinyxml2::XMLDocument &);

  unsigned runs {0};
  std::chrono::milliseconds elapsed_time {0};
  double success_rate {0.0};

  ultra::fitnd fit_mean {};
  ultra::fitnd fit_std_dev {};

  ultra::fitnd best_fit {-std::numeric_limits<double>::infinity()};
  std::string best_prg {};

  std::vector<
    std::pair<unsigned, ultra::model_measurements<ultra::fitnd>>> elite {};
};

namespace rs
{

enum class exec_mode {run, summary};

enum class task_state {queued, running, finished, failed, cancelled};

struct task_status
{
  task_state state {task_state::queued};
  std::string error {};
};

/// Shares dataset task status between the search worker and live reports.
class dataset_activity
{
public:
  void set(task_state, std::string error = {});
  [[nodiscard]] task_status status() const;

private:
  mutable std::mutex mutex_;
  task_status status_ {};
};

struct settings
{
  ultra::src::dataframe::params params {};

  unsigned generations {100};
  unsigned runs {1};
  ultra::model_measurements<double> threshold {};
  bool common_random_numbers {false};
};

struct entry
{
  std::string name {};
  std::filesystem::path dataset {};
  std::filesystem::path xml_summary {};

  settings conf {};
  summary_data current {};
  summary_data reference {};
  std::shared_ptr<dataset_activity> activity {};
};

using collection_t = std::vector<entry>;

[[nodiscard]] std::expected<ultra::model_measurements<double>, std::string>
extract_threshold(std::string_view);
[[nodiscard]] settings read_settings(const std::filesystem::path &,
                                     const settings & = {});
[[nodiscard]] collection_t setup_collection(std::filesystem::path,
                                            std::filesystem::path,
                                            exec_mode,
                                            const settings & = {});

[[nodiscard]] ultra::search_log make_search_log(
  const std::filesystem::path &, const std::filesystem::path & = {});

namespace run
{

struct options
{
  collection_t collection {};
  unsigned threads {0};  // 0 = auto (`hardware_threads()`)
  std::filesystem::path html_output {};
  unsigned interval {10};
};

[[nodiscard]] bool start(options);

}  // namespace run

namespace summary
{

struct options
{
  collection_t collection {};
  std::filesystem::path html_output {"summary.html"};
};

[[nodiscard]] bool start(options);

}  // namespace summary

}  // namespace rs

struct side_snapshot
{
  struct elite_entry
  {
    unsigned id {0};
    double fit {std::numeric_limits<double>::quiet_NaN()};
    double acc {std::numeric_limits<double>::quiet_NaN()};
  };

  unsigned runs {0};
  double success_rate {std::numeric_limits<double>::quiet_NaN()};
  double time_per_run {std::numeric_limits<double>::quiet_NaN()};

  double fit_best {std::numeric_limits<double>::quiet_NaN()};
  double fit_mean {std::numeric_limits<double>::quiet_NaN()};
  double fit_std_dev {std::numeric_limits<double>::quiet_NaN()};

  std::vector<elite_entry> elite {};
  std::string best_prg {};
};

struct dataset_snapshot
{
  std::string name {};
  unsigned capacity_runs {0};
  bool has_threshold {false};
  std::string remaining_label {};
  std::optional<rs::task_status> status {};

  side_snapshot current {};
  side_snapshot reference {};
};

struct results_snapshot
{
  explicit results_snapshot(const rs::collection_t &,
                            rs::exec_mode = rs::exec_mode::summary);

  bool references_available {false};
  bool show_success_rate {false};
  std::vector<dataset_snapshot> datasets {};
};

/// Estimates remaining search time in seconds for a dataset snapshot.
[[nodiscard]] std::optional<double> estimated_time_remaining(
  const dataset_snapshot &);

/// Formats a human-readable remaining-time label for a dataset snapshot.
[[nodiscard]] std::string remaining_time_label(const dataset_snapshot &);

}  // namespace ultra::wopr

#endif  // include guard
