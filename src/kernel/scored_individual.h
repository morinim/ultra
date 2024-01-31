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

#if !defined(ULTRA_SCORED_INDIVIDUAL_H)
#define      ULTRA_SCORED_INDIVIDUAL_H

#include "kernel/individual.h"
#include "kernel/fitness.h"
#include "kernel/problem.h"

namespace ultra
{
///
/// An individual / fitness pair.
///
template<Individual I, Fitness F>
struct scored_individual
{
  // --- Constructor and support functions ---
  scored_individual() = default;
  scored_individual(const I &, const F &);

  // --- Misc ---
  [[nodiscard]] bool empty() const;

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &, const problem &);
  [[nodiscard]] bool save(std::ostream &) const;

  // --- Data members ---
  I ind {};
  F fit {lowest<F>()};
};

template<Individual I, Fitness F>
[[nodiscard]] bool operator<(const scored_individual<I, F> &,
                             const scored_individual<I, F> &);
template<Individual I, Fitness F>
[[nodiscard]] bool operator<=(const scored_individual<I, F> &,
                              const scored_individual<I, F> &);
template<Individual I, Fitness F>
[[nodiscard]] bool operator>(const scored_individual<I, F> &,
                             const scored_individual<I, F> &);
template<Individual I, Fitness F>
[[nodiscard]] bool operator>=(const scored_individual<I, F> &,
                              const scored_individual<I, F> &);

#include "kernel/scored_individual.tcc"

}  // namespace ultra

#endif  // include guard
