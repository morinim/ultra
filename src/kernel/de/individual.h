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

#if !defined(ULTRA_DE_INDIVIDUAL_H)
#define      ULTRA_DE_INDIVIDUAL_H

#include "kernel/vector_individual.h"
#include "kernel/interval.h"

namespace ultra::de
{

///
/// An individual optimized for differential evolution.
///
///
/// \note
/// This class implements a value type with no internal synchronisation. Any
/// operation that mutates the individual is not thread-safe and must not run
/// concurrently with any member function unless externally synchronised.
///
/// \see
/// - https://github.com/morinim/ultra/wiki/bibliography#4
/// - https://github.com/morinim/ultra/wiki/bibliography#5
///
class individual final : public vector_individual<D_DOUBLE>
{
public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const ultra::problem &p) : vector_individual(p) {}

  // ---- Modifiers ----
  using vector_individual::apply_each;
  using vector_individual::operator=;

  // ---- Recombination operators ----
  [[nodiscard]] individual crossover(double, const interval<double> &,
                                     const individual &, const individual &,
                                     const individual &) const;

private:
  // ---- Printing ----
  void print_impl(std::ostream &, out::print_format_t) const override;
};  // class individual

// ---- Non-member functions ----
[[nodiscard]] double distance(const individual &, const individual &);

}  // namespace ultra::de

#include "kernel/de/individual_format.h"

#endif  // include guard
