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
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SEARCH_STATS_TCC)
#define      ULTRA_SEARCH_STATS_TCC

///
/// Updates the search statistics with data from the latest run.
///
/// \param[in] lr_best_prg     best individual from the evolution run just
///                            finished
/// \param[in] lr_measurements measurements from the last run
/// \param[in] lr_elapsed      time taken for the last evolutionary run
/// \param[in] threshold       used to identify good runs
///
template<Individual I, Fitness F>
void search_stats<I, F>::update(const I &lr_best_prg,
                                const model_measurements<F> &lr_measurements,
                                std::chrono::milliseconds lr_elapsed,
                                const model_measurements<F> &threshold)
{
  if (lr_measurements > best_measurements)
  {
    best_individual = lr_best_prg;
    best_measurements = lr_measurements;
    best_run = runs;
  }

  if ((threshold.fitness.has_value() || threshold.accuracy.has_value())
      && lr_measurements > threshold)
    good_runs.insert(runs);

  using std::isfinite;
  if (const auto fit(*lr_measurements.fitness); isfinite(fit))
    fitness_distribution.add(fit);

  elapsed += lr_elapsed;

  ++runs;

  Ensures(good_runs.empty() || good_runs.contains(best_run));
}

#endif  // include guard
