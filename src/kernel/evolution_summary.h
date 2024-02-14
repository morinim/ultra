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

#if !defined(ULTRA_EVOLUTION_SUMMARY_H)
#define      ULTRA_EVOLUTION_SUMMARY_H

#include <chrono>

#include "kernel/analyzer.h"
#include "kernel/evolution_status.h"
#include "kernel/model_measurements.h"

namespace ultra
{
///
/// A summary of information about evolution (results, statistics...).
///
/// Part of the information (`status`) supports concurrent access and is keep
/// up to date while evolution is ongoing; the remaining part is calculated at
/// the end of evolution.
///
template<Individual I, Fitness F>
class summary
{
public:
  // --- Constructor and support functions ---
  summary() = default;

  void clear();

  [[nodiscard]] evolution_status<I, F> starting_status();

  // --- Concurrency aware functions ---
  void update_if_better(scored_individual<I, F>);
  [[nodiscard]] scored_individual<I, F> best() const;

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &, const problem &);
  [[nodiscard]] bool save(std::ostream &) const;

  // --- Data members ---
  analyzer<I, F> az {};

  model_measurements<F> score {};

  /// Time elapsed from evolution beginning.
  std::chrono::milliseconds elapsed {0};

  unsigned generation {0};

private:
  mutable ignore_copy<std::mutex> mutex_ {};

  scored_individual<I, F> best_ {};
};

#include "kernel/evolution_summary.tcc"

}  // namespace ultra

#endif  // include guard
