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
/// Updates statistics with the results of a completed run.
///
/// \param[in] lr_best_prg     best individual produced by the completed run
/// \param[in] lr_measurements measurements associated with the best individual
/// \param[in] lr_elapsed      duration of the completed run
/// \param[in] threshold       measurements threshold used to identify
///                            successful runs
///
/// If `threshold` defines at least one criterion (fitness or accuracy),
/// the run is marked as successful when `lr_measurements >= threshold`.
///
template<Individual I, Fitness F>
void search_stats<I, F>::update(const I &lr_best_prg,
                                const model_measurements<F> &lr_measurements,
                                std::chrono::milliseconds lr_elapsed,
                                const model_measurements<F> &threshold)
{
  const std::size_t initial_run_id(runs());

  // upper_bound finds the first element that is "strictly greater" than our
  // value according to the comparator.
  // Since we use `std::greater`, it finds the first element SMALLER than
  // `new_run`.
  auto it(std::ranges::upper_bound(stats_, lr_measurements,
                                   std::greater{},
                                   &run_summary::best_measurements));

  run_summary new_run{initial_run_id, lr_best_prg, lr_measurements};
  stats_.insert(it, std::move(new_run));

  if (!threshold.empty() && lr_measurements >= threshold)
    good_runs.insert(initial_run_id);

  using std::isfinite;
  if (const auto fit(lr_measurements.fitness); fit && isfinite(*fit))
    fitness_distribution.add(*fit);

  elapsed += lr_elapsed;

  //Ensures(good_runs.empty() || good_runs.contains(best_run()));
}

///
/// Returns the identifier of the best run.
///
/// \pre `runs() > 0`
///
/// The best run is the one whose measurements compare highest according
/// to `model_measurements<F>` ordering.
///
template<Individual I, Fitness F>
std::size_t search_stats<I, F>::best_run() const noexcept
{
  Expects(runs());
  return stats_.front().run;
}

///
/// Returns the best individual observed across all runs.
///
/// \pre `runs() > 0`
///
template<Individual I, Fitness F>
const I &search_stats<I, F>::best_individual() const noexcept
{
  Expects(runs());
  return stats_.front().best_individual;
}

///
/// Returns the measurements associated with the best run.
///
/// \pre `runs() > 0`
///
template<Individual I, Fitness F>
const model_measurements<F> &
search_stats<I, F>::best_measurements() const noexcept
{
  Expects(runs());
  return stats_.front().best_measurements;
}

///
/// Returns the number of completed runs.
///
template<Individual I, Fitness F>
std::size_t search_stats<I, F>::runs() const noexcept
{
  return stats_.size();
}

///
/// Returns the fraction of successful runs.
///
/// A run is considered successful if it satisfies the threshold specified
/// during `update()`.
///
/// Returns `0` when no runs have been recorded.
///
template<Individual I, Fitness F>
double search_stats<I, F>::success_rate() const noexcept
{
  const auto solutions(static_cast<double>(good_runs.size()));

  return runs() ? solutions / static_cast<double>(runs()) : 0;
}

#endif  // include guard
