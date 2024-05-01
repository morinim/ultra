/**
 *  \file
 *  \remark This file is part of VITA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SEARCH_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SEARCH_TCC)
#define      ULTRA_SEARCH_TCC

///
/// \param[in] prob the problem we're working on. The lifetime of `prob` must
///                 exceed lifetime of `this` class
/// \param[in] eva  evaluator used during evolution. Must be copyable and
///                 could be used to build a proxy evaluator.
///
template<template<class> class ES, Evaluator E>
basic_search<ES, E>::basic_search(problem &prob, E eva)
  : es_(prob, evaluator_proxy(eva, prob.params.cache.size)),
    prob_(prob)
{
  Ensures(is_valid());
}

template<Evaluator E>
search<E>::search(problem &prob, E eva) : basic_search<alps_es, E>(prob, eva)
{
}

///
/// Performs basic initialization before the search.
///
/// The default behaviour involve:
/// - tuning of the search parameters;
/// - possibly loading cached value for the evaluator.
///
/// \remark
/// Called at the beginning of the first run (i.e. only one time even for a
/// multiple-run search).
///
template<template<class> class ES, Evaluator E>
void basic_search<ES, E>::init()
{
  tune_parameters();

  load();
}

///
/// Sets a callback function executed at the end of every generation.
///
/// \param[in] f callback function
/// \return      a reference to *this* object (method chaining / fluent
///              interface)
///
template<template<class> class ES, Evaluator E>
basic_search<ES, E> &basic_search<ES, E>::after_generation(
  after_generation_callback_t f)
{
  after_generation_callback_ = std::move(f);
  return *this;
}

///
/// Tries to tune search parameters for the current problem.
///
template<template<class> class ES, Evaluator E>
void basic_search<ES, E>::tune_parameters()
{
  // The `shape` function modifies the default parameters with
  // strategy-specific values.
  const auto dflt(es_.shape(parameters().init()));
  const auto constrained(prob_.params);

  if (!constrained.slp.code_length)
    prob_.params.slp.code_length = dflt.slp.code_length;

  if (constrained.evolution.elitism < 0.0)
    prob_.params.evolution.elitism = dflt.evolution.elitism;

  if (constrained.evolution.p_mutation < 0.0)
    prob_.params.evolution.p_mutation = dflt.evolution.p_mutation;

  if (constrained.evolution.p_cross < 0.0)
    prob_.params.evolution.p_cross = dflt.evolution.p_cross;

  if (!constrained.evolution.brood_recombination)
    prob_.params.evolution.brood_recombination =
      dflt.evolution.brood_recombination;

  if (!constrained.population.init_subgroups)
    prob_.params.population.init_subgroups = dflt.population.init_subgroups;

  if (!constrained.population.individuals)
    prob_.params.population.individuals = dflt.population.individuals;

  if (!constrained.population.min_individuals)
    prob_.params.population.min_individuals = dflt.population.min_individuals;

  if (!constrained.evolution.tournament_size)
    prob_.params.evolution.tournament_size = dflt.evolution.tournament_size;

  if (!constrained.evolution.mate_zone)
    prob_.params.evolution.mate_zone =
      std::max<std::size_t>(prob_.params.population.individuals / 5, 4);

  if (!constrained.evolution.generations)
    prob_.params.evolution.generations = dflt.evolution.generations;

  if (!constrained.evolution.max_stuck_gen)
    prob_.params.evolution.max_stuck_gen = dflt.evolution.max_stuck_gen;

  Ensures(prob_.params.is_valid(true));
}

///
/// Executes a given number of evolutionary-runs possibly saving good runs.
///
/// \param[in] n         number of runs
/// \param[in] threshold used to identify successfully learned (matched,
///                      classified, resolved...) examples
/// \return              a summary of the search
///
template<template<class> class ES, Evaluator E>
search_stats<typename basic_search<ES, E>::individual_t,
             typename basic_search<ES, E>::fitness_t>
basic_search<ES, E>::run(unsigned n,
                         const model_measurements<fitness_t> &threshold)
{
  init();

  auto shake([this](unsigned g) { return vs_->shake(g); });
  search_stats<individual_t, fitness_t> stats;

  for (unsigned r(0); r < n; ++r)
  {
    vs_->training_setup(r);

    evolution evo(es_);
    evo.after_generation(after_generation_callback_);
    evo.shake_function(shake);
    const auto run_summary(evo.run());

    vs_->validation_setup(r);

    // Update the search statistics (possibly using the validation setup).
    if (const auto prg = run_summary.best().ind; !prg.empty())
      stats.update(prg, calculate_metrics(prg), run_summary.elapsed,
                   threshold);
  }

  return stats;
}

///
/// Calculates and stores the fitness of the best individual so far.
///
/// \param[in] prg best individual from the evolution run just finished
/// \return        measurements about the individual
///
/// Specializations of this method can calculate further / distinct
/// problem-specific metrics regarding the candidate solution.
///
/// If a validation set / simulation is available, it's used for the
/// calculations.
///
template<template<class> class ES, Evaluator E>
model_measurements<typename basic_search<ES, E>::fitness_t>
basic_search<ES, E>::calculate_metrics(const individual_t &prg) const
{
  model_measurements<fitness_t> ret;

  ret.fitness = es_.evaluator()(prg);

  return ret;
}

///
/// Sets the active validation strategy.
///
/// \param[in] args parameters for the validation strategy
/// \return         a reference to the search class (used for method chaining)
///
template<template<class> class ES, Evaluator E>
template<class V, class... Args>
basic_search<ES, E> &basic_search<ES, E>::validation_strategy(Args && ...args)
{
  vs_ = std::make_unique<V>(std::forward<Args>(args)...);
  return *this;
}

///
/// Loads the saved evaluation cache from a file (if available).
///
/// \return `true` if the object is correctly loaded
///
template<template<class> class ES, Evaluator E>
bool basic_search<ES, E>::load()
{
  if (prob_.params.cache.serialization_file.empty())
    return true;

  std::ifstream in(prob_.params.cache.serialization_file);
  if (!in)
    return false;

  if (prob_.params.cache.size)
  {
    if (!es_.evaluator().load_cache(in))
      return false;

    ultraINFO << "Loading cache from "
              << prob_.params.cache.serialization_file;
  }

  return true;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<template<class> class ES, Evaluator E>
bool basic_search<ES, E>::is_valid() const
{
  return true;
}

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
