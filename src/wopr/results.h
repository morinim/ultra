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

#include "imgui_app.h"

#include "kernel/fitness.h"
#include "kernel/model_measurements.h"
#include "kernel/gp/src/dataframe.h"

#include <chrono>
#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace tinyxml2 { class XMLDocument; }

namespace ultra::wopr
{

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
  double best_accuracy {-std::numeric_limits<double>::infinity()};

  unsigned best_run {};
  std::string best_prg {};

  std::set<unsigned> good_runs {};
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

struct data
{
  data(const std::filesystem::path &ds, const std::filesystem::path &xs,
       const settings &c, const summary_data &r)
    : dataset(ds), xml_summary(xs), conf(c), reference(r)
  {
  }

  const std::filesystem::path dataset;
  const std::filesystem::path xml_summary;

  const settings conf;
  summary_data current {};
  const summary_data reference;
};

using collection_t = std::vector<std::pair<const std::string, data>>;

[[nodiscard]] settings read_settings(const std::filesystem::path &,
                                     const settings & = {});
[[nodiscard]] collection_t setup_collection(std::filesystem::path,
                                            std::filesystem::path,
                                            exec_mode,
                                            const settings & = {});

namespace run
{

struct options
{
  collection_t collection {};
  bool nogui {false};
  bool imgui_demo {false};
};

[[nodiscard]] bool start(const imgui_app::program::settings &, options);

}  // namespace run

namespace summary
{

struct options
{
  collection_t collection {};
  bool imgui_demo {false};
};

void start(const imgui_app::program::settings &, options);

}  // namespace summary

}  // namespace rs

}  // namespace ultra::wopr

#endif  // include guard
