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

#include "kernel/model_measurements.h"

namespace ultra
{

template<Individual I, Fitness F>
struct search_stats
{
  void update(const I &, const model_measurements<F> &,
              std::chrono::milliseconds, const model_measurements<F> &);

  I best_individual {};
  model_measurements<F> best_measurements {};

  distribution<F> fitness_distribution {};
  std::set<unsigned> good_runs {};

  /// Time elapsed from search beginning.
  std::chrono::milliseconds elapsed {0};

  unsigned best_run {0};  /// index of the run giving the best solution
  unsigned runs     {0};  /// number of runs performed so far
};

#include "kernel/search_stats.tcc"
}  // namespace ultra

#endif  // include guard
