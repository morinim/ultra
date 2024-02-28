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
#include "kernel/evolution_summary.h"
#include "kernel/layered_population.h"
#include "utility/timer.h"

namespace ultra
{

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

  explicit evolution(const S &);

  summary<individual_t, fitness_t> run();

  [[nodiscard]] bool is_valid() const;

private:
  [[nodiscard]] bool stop_condition() const;

  // *** Data members ***
  summary<individual_t, fitness_t> sum_;

  layered_population<individual_t> pop_;
  S es_;
};

#include "kernel/evolution.tcc"
}  // namespace ultra

#endif  // include guard
