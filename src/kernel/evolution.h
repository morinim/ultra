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

#include <future>

#include "kernel/evolution_strategy.h"
#include "kernel/evolution_summary.h"
#include "kernel/layered_population.h"

#include "utility/term.h"
#include "utility/timer.h"

namespace ultra
{

template<Individual I, Fitness F>
using after_generation_callback_t =
  std::function<void(const layered_population<I> &, const summary<I, F> &)>;

///
/// Progressively evolves a population of programs over a series of
/// generations.
///
/// The evolutionary search uses the Darwinian principle of natural selection
/// (survival of the fittest) and analogs of various naturally occurring
/// operations, including crossover (sexual recombination), mutation...
///
template<Strategy S>
class evolution
{
public:
  using individual_t = typename S::individual_t;
  using fitness_t = typename S::fitness_t;

  using after_generation_callback_t =
    ultra::after_generation_callback_t<individual_t, fitness_t>;

  explicit evolution(S);

  summary<individual_t, fitness_t> run();

  evolution &after_generation(after_generation_callback_t);
  evolution &shake_function(const std::function<bool(unsigned)> &);

  [[nodiscard]] bool is_valid() const;

private:
  void log_evolution() const;
  void print(bool, std::chrono::milliseconds, timer *) const;
  [[nodiscard]] bool stop_condition() const;

  // *** Data members ***
  summary<individual_t, fitness_t> sum_ {};

  layered_population<individual_t> pop_;
  S es_;
  std::function<bool(unsigned)> shake_ {};

  after_generation_callback_t after_generation_callback_ {};
};

#include "kernel/evolution.tcc"
}  // namespace ultra

#endif  // include guard
