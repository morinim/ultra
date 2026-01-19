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

#if !defined(ULTRA_EVOLUTION_H)
#define      ULTRA_EVOLUTION_H

#include "kernel/evolution_strategy.h"
#include "kernel/layered_population.h"
#include "kernel/search_log.h"

#include "utility/term.h"
#include "utility/thread_pool.h"
#include "utility/timer.h"

namespace ultra
{

template<Individual I, Fitness F>
using after_generation_callback_t =
  std::function<void(const layered_population<I> &, const summary<I, F> &)>;

///
/// The evolution class orchestrates the iterative improvement of a population
/// of candidate solutions over multiple generations.
///
/// \tparam E evaluator type used to score individuals
///
/// It is parameterised by an `Evaluator`, from which it derives:
/// - the individual representation;
/// - the fitness type;
/// - the evaluation semantics.
///
/// The evolution process is strategy-driven: the actual evolutionary operators
/// (selection, variation, replacement) are delegated to an evolution strategy
/// supplied as a template parameter to \c run().
///
/// ### Responsibilities
/// - Initialise and validate the population.
/// - Execute the evolutionary loop across generations.
/// - Dispatch parallel tasks to evolve subpopulations.
/// - Monitor progress and termination conditions.
/// - Collect summary statistics and optional logs.
/// - Invoke user-defined hooks and callbacks.
///
/// ### Concurrency model
/// Evolution supports parallel execution through a thread pool. Each
/// generation may evolve multiple subpopulations concurrently, while
/// cooperative cancellation is handled via `std::stop_source`.
///
/// ### Customisation points
/// Users can customise behaviour via:
/// - evolution strategies;
/// - per-generation callbacks;
/// - shake functions;
/// - logging facilities;
/// - external stop sources.
///
/// \see layered_population
/// \see evolution_strategy
///
template<Evaluator E>
class evolution
{
public:
  using individual_t = evaluator_individual_t<E>;
  using fitness_t = evaluator_fitness_t<E>;

  using after_generation_callback_t =
    ultra::after_generation_callback_t<individual_t, fitness_t>;

  // ---- Constructor ----
  explicit evolution(const problem &, E &);

  // ---- Run ----
  template<template<class> class ES>
  summary<individual_t, fitness_t> run();

  // ---- Callback and configuration methods ----
  evolution &after_generation(after_generation_callback_t);
  evolution &logger(search_log &);
  evolution &shake_function(const std::function<bool(unsigned)> &);
  evolution &stop_source(std::stop_source);
  evolution &tag(const std::string &);

  // ---- Misc ----
  [[nodiscard]] bool is_valid() const;

private:
  void print(bool, std::chrono::milliseconds, timer *) const;
  [[nodiscard]] bool stop_condition() const;

  // *** Data members ***
  layered_population<individual_t> pop_;
  E &eva_;

  summary<individual_t, fitness_t> sum_ {};

  std::function<bool(unsigned)> shake_ {};

  after_generation_callback_t after_generation_callback_ {};

  mutable search_log *search_log_ {nullptr};
  std::stop_source external_stop_source_ {std::nostopstate};
  std::string tag_ {};
};

#include "kernel/evolution.tcc"
}  // namespace ultra

#endif  // include guard
