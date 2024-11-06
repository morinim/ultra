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

#if !defined(ULTRA_SEARCH_LOG_H)
#define      ULTRA_SEARCH_LOG_H

#include <filesystem>
#include <fstream>

#include "kernel/evolution_summary.h"

namespace ultra
{

class search_log
{
public:
  static constexpr auto default_dynamic_file {"dynamic.txt"};
  static constexpr auto default_layers_file {"layers.txt"};
  static constexpr auto default_population_file {"population.txt"};

  template<Population P, Fitness F> void save_snapshot(
    const P &, const summary<std::ranges::range_value_t<P>, F> &);

  /// A base common path for log files.
  /// \note
  /// A single log file can overwrite this path specifying an absolute path.
  std::filesystem::path base_dir {};

  /// Name of the file used to save "real-time" information.
  /// \note
  /// An empty string disable logging.
  std::filesystem::path dynamic_file_path {};

  /// Name of the log file used to save layer-specific information.
  /// \note
  /// An empty string disable logging.
  std::filesystem::path layers_file_path {};

  /// Name of the log file used to save population-specific information.
  /// \note
  /// An empty string disable logging.
  /// \warning
  /// Enabling this log with large populations has a big performance impact.
  std::filesystem::path population_file_path {};

private:
  bool open();

  template<Individual I, Fitness F> void save_dynamic(
    const summary<I, F> &, const distribution<F> &);
  template<Population P, Fitness F> void save_layers(
    const P &, const summary<std::ranges::range_value_t<P>, F> &);
  template<Fitness F> void save_population(unsigned, const distribution<F> &);

  ignore_copy<std::ofstream> dynamic_file;
  ignore_copy<std::ofstream> layers_file;
  ignore_copy<std::ofstream> population_file;
};  // search_log

#include "kernel/search_log.tcc"

}  // namespace ultra

#endif  // include guard
