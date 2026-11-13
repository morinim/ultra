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

struct settings
{
  ultra::src::dataframe::params params {};

  unsigned generations {100};
  unsigned runs {1};
  ultra::model_measurements<double> threshold {};
};

struct entry
{
  std::string name {};
  std::filesystem::path dataset {};
  std::filesystem::path xml_summary {};

  settings conf {};
  summary_data current {};
  summary_data reference {};
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
  bool nogui {false};
  bool imgui_demo {false};
};

[[nodiscard]] bool start(options);

}  // namespace run

namespace summary
{

struct options
{
  collection_t collection {};
  bool imgui_demo {false};
  exec_mode mode {exec_mode::summary};
};

#if defined(ULTRA_WOPR_GUI)
void start(options);
#endif

}  // namespace summary

}  // namespace rs

}  // namespace ultra::wopr

#endif  // include guard
