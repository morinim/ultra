/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_EVOLUTION_CALLBACKS_H)
#define      ULTRA_EVOLUTION_CALLBACKS_H

#include "kernel/evaluator.h"
#include "kernel/parameters.h"

#include <functional>

namespace ultra
{

template<Individual I> class layered_population;
template<Individual I, Fitness F> struct scored_individual;
template<Individual I, Fitness F> class summary;

template<Individual I, Fitness F>
using after_generation_callback_t =
  std::function<void(const layered_population<I> &, const summary<I, F> &)>;

template<Individual I, Fitness F>
using on_new_best_callback_t =
  std::function<void (const scored_individual<I, F> &)>;

/// Refinement backend callback.
///
/// Backends are responsible for deciding whether a refined individual should
/// replace the original one (return `std::nullopt` to leave the population
/// unchanged).
///
/// When refinement is performed in parallel, this callback may be invoked
/// concurrently for distinct candidate individuals. Implementations must be
/// thread-safe and must synchronize access to shared state they capture.
template<Evaluator E>
using refinement_callback_t =
  std::function<std::optional<evaluator_fitness_t<E>>(
    evaluator_individual_t<E> &, const E &,
    const parameters::refinement_parameters &)>;

using shake_function_callback_t = std::function<evaluation_context(unsigned)>;

}  // namespace ultra

#endif  // include guard
