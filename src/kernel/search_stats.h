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

#if !defined(ULTRA_SEARCH_STATS_H)
#define      ULTRA_SEARCH_STATS_H

#include "kernel/distribution.h"
#include "kernel/fitness.h"
#include "kernel/individual.h"
#include "kernel/model_measurements.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <span>
#include <vector>

namespace ultra
{

///
/// Collects statistics across multiple independent search runs.
///
/// \tparam I individual type
/// \tparam F fitness type
///
/// The class stores per-run summaries ordered by best performance, tracks the
/// distribution of fitness values and records runs that satisfy a
/// user-defined success threshold.
///
template<Individual I, Fitness F>
class search_stats
{
public:
  /// Summary information for a single run.
  struct run_summary
  {
    std::size_t run {};  // run identifier
    I best_individual {};  // best individual produced by the run
    model_measurements<F> best_measurements {};  // measurements associated
  };                                             //  with the best individual

  void update(const I &, const model_measurements<F> &,
              std::chrono::milliseconds);

  [[nodiscard]] std::size_t best_run() const noexcept;
  [[nodiscard]] const I &best_individual() const noexcept;
  [[nodiscard]] std::set<std::size_t> good_runs(
    const model_measurements<F> &) const;
  [[nodiscard]] const model_measurements<F> &best_measurements() const noexcept;
  [[nodiscard]] std::size_t runs() const noexcept;
  [[nodiscard]] double success_rate(
    const model_measurements<F> &) const noexcept;
  [[nodiscard]] std::span<const run_summary> elite_runs(
    double = 0.05) const noexcept;

  /// Distribution of finite fitness values observed across runs.
  ///
  /// Only runs with finite fitness values contribute to the distribution.
  distribution<F> fitness_distribution {};

  /// Identifiers of runs that satisfied the success threshold.
  ///
  /// Run identifiers correspond to the order in which runs were recorded.

  /// Total elapsed time across all runs.
  ///
  /// This is the sum of the durations passed to `update()`.
  std::chrono::milliseconds elapsed {0};

private:
  std::vector<run_summary> stats_ {};
};

#include "kernel/search_stats.tcc"
}  // namespace ultra

#endif  // include guard
