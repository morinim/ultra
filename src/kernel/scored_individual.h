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
/// Associates an individual with its fitness value.
///
/// \tparam I type modelling an individual
/// \tparam F type modelling a fitness value
///
/// `scored_individual` is a lightweight value type used to represent the
/// result of evaluating an individual. It is primarily intended for **ranking,
/// selection, and replacement** operations in evolutionary algorithms.
///
/// The class supports *ordering by fitness* (via `operator<=>`) but
/// deliberately does **not** define equality. Fitness values are typically
/// floating-point and exact equality comparisons would be semantically
/// misleading.
///
/// \note
/// Since equality is not defined, `scored_individual` does not model a totally
/// ordered type and cannot be used with default `std::ranges::less`.
/// Algorithms that require ordering should provide an explicit comparator or
/// projection (e.g. comparing the `fit` member).
///
template<Individual I, Fitness F>
struct scored_individual
{
  // ---- Constructor and support functions ----
  scored_individual() = default;
  scored_individual(const I &, const F &);

  // ---- Capacity ----
  [[nodiscard]] bool empty() const noexcept;

  // ---- Serialization ----
  [[nodiscard]] bool load(std::istream &, const problem &);
  [[nodiscard]] bool save(std::ostream &) const;

  // ---- Data members ----
  I ind {};
  F fit {lowest<F>()};
};


// ---- Non-member functions ----
template<Individual I, Fitness F>
[[nodiscard]] auto operator<=>(const scored_individual<I, F> &,
                               const scored_individual<I, F> &) noexcept;

#include "kernel/scored_individual.tcc"

}  // namespace ultra

#endif  // include guard
