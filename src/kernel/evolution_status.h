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

#if !defined(ULTRA_EVOLUTION_STATUS_H)
#define      ULTRA_EVOLUTION_STATUS_H

#include "kernel/scored_individual.h"
#include "utility/misc.h"

namespace ultra
{
///
/// A collection of information about the ongoing search.
///
/// \remark
/// Supports concurrency-safe operations.
///
template<Individual I, Fitness F>
class evolution_status
{
public:
  // --- Constructor and support functions ---
  evolution_status() = default;
  explicit evolution_status(const scored_individual<I, F> &);
  explicit evolution_status(const unsigned *);

  // --- Misc ---
  [[nodiscard]] scored_individual<I, F> best() const;
  [[nodiscard]] unsigned last_improvement() const;
  bool update_if_better(const scored_individual<I, F> &);

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &, const problem &);
  [[nodiscard]] bool save(std::ostream &) const;

private:
  mutable std::shared_ptr<std::shared_mutex> pmutex_
  {std::make_shared<std::shared_mutex>()};

  scored_individual<I, F> best_ {};

  // Current generation.
  const unsigned *generation_ {nullptr};

  // This is the generation the last improvement occurred in.
  unsigned last_improvement_ {0};

public:
  /// Number of crossovers performed.
  copyable_atomic<std::uintmax_t> crossovers {0};

  /// Number of mutations performed.
  copyable_atomic<std::uintmax_t> mutations {0};
};

#include "kernel/evolution_status.tcc"

}  // namespace ultra

#endif  // include guard
