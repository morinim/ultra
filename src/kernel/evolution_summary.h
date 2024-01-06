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
#include "kernel/model_measurements.h"
#include "kernel/problem.h"

namespace ultra
{
///
/// A summary of evolution (results, statistics...).
///
template<Individual I, Fitness F>
class summary
{
public:
  // --- Constructor and support functions ---
  summary() = default;

  void clear();

  void update_best(const I &, const F &);

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &, const problem &);
  [[nodiscard]] bool save(std::ostream &) const;

  // --- Data members ---
  analyzer<I, F> az {};

  struct
  {
    I                  solution {};
    model_measurements<F> score {};
  } best {};

  /// Time elapsed from evolution beginning.
  std::chrono::milliseconds elapsed {0};

  /// Number of crossovers performed.
  std::uintmax_t crossovers {0};

  /// Number of mutations performed.
  std::uintmax_t mutations {0};

  unsigned gen {0};
  unsigned last_imp {0};
};

#include "kernel/evolution_summary.tcc"

}  // namespace ultra

#endif  // include guard
