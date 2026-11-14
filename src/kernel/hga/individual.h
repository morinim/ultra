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

#if !defined(ULTRA_HGA_INDIVIDUAL_H)
#define      ULTRA_HGA_INDIVIDUAL_H

#include "kernel/vector_individual.h"

#include <functional>

namespace ultra::hga
{

///
/// An heterogeneous GA-individual.
///
class individual final : public vector_individual<value_t>
{
private:
  class modify_proxy;

public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const ultra::problem &);

  // ---- Modifiers ----
  template<class F>
  requires
    std::invocable<F &, individual::modify_proxy &>
    && std::same_as<std::invoke_result_t<F &, individual::modify_proxy&>, void>
  void modify(F &&);

  using vector_individual::operator=;

  // ---- Recombination operators ----
  unsigned mutation(const problem &, double = 1.0);
  friend individual crossover(const problem &,
                              const individual &, const individual &);

private:
  // ---- Hashing ----
  [[nodiscard]] std::vector<std::byte> pack() const;
  [[nodiscard]] hash_t hash() const final;

  // ---- Printing ----
  void print_impl(std::ostream &, out::print_format_t) const override;
};  // class individual


// ---- Non-member functions ----
[[nodiscard]] unsigned distance(const individual &, const individual &);

[[nodiscard]] individual crossover(const problem &,
                                   const individual &, const individual &);

#include "kernel/hga/individual.tcc"

}  // namespace ultra::hga

#include "kernel/hga/individual_format.h"

#endif  // include guard
