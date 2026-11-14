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

#if !defined(ULTRA_GA_INDIVIDUAL_H)
#define      ULTRA_GA_INDIVIDUAL_H

#include "kernel/vector_individual.h"

namespace ultra::ga
{

///
/// An GA-individual optimized for combinatorial optimization.
///
/// \note
/// This class implements a value type with no internal synchronisation. Any
/// operation that mutates the individual is not thread-safe and must not run
/// concurrently with any member function unless externally synchronised.
///
class individual final : public vector_individual<D_INT>
{
public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const ultra::problem &p) : vector_individual(p) {}

  // ---- Modifiers ----
  using vector_individual::apply_each;
  using vector_individual::operator=;

  // ---- Recombination operators ----
  unsigned mutation(const problem &, double = 1.0);
  friend individual crossover(const problem &,
                              const individual &, const individual &);

private:
  // ---- Printing ----
  void print_impl(std::ostream &, out::print_format_t) const override;
};  // class individual


// ---- Non-member functions ----
[[nodiscard]] unsigned distance(const individual &, const individual &);

// Recombination operators.
[[nodiscard]] individual crossover(const problem &,
                                   const individual &, const individual &);

}  // namespace ultra::ga

#include "kernel/ga/individual_format.h"

#endif  // include guard
