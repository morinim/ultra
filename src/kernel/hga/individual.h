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

#include "kernel/individual.h"
#include "kernel/problem.h"

#include <functional>

namespace ultra::hga
{

///
/// An heterogeneous GA-individual.
///
class individual final : public ultra::individual
{
private:
  class modify_proxy;

public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const ultra::problem &);

  // ---- Member types ----
  using genome_t       = std::vector<value_t>;
  using const_iterator = genome_t::const_iterator;
  using iterator       = genome_t::iterator;
  using value_type     = genome_t::value_type;

  // ---- Iterators ----
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  // ---- Element access ----
  [[nodiscard]] const value_type &operator[](std::size_t) const;

  // ---- Modifiers ----
  template<class F>
  requires
    std::invocable<F &, individual::modify_proxy &>
    && std::same_as<std::invoke_result_t<F &, individual::modify_proxy&>, void>
  void modify(F &&);

  // ---- Recombination operators ----
  unsigned mutation(const problem &);
  friend individual crossover(const problem &,
                              const individual &, const individual &);

  // ---- Capacity ----
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t parameters() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

  // ---- Misc ----
  [[nodiscard]] operator std::vector<value_type>() const noexcept;

  individual &operator=(const std::vector<value_type> &);

  [[nodiscard]] bool is_valid() const;

private:
  // ---- Private support methods ----
  [[nodiscard]] std::vector<std::byte> pack() const;
  [[nodiscard]] hash_t hash() const final;

  // Serialization.
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  // ---- Private data members ----
  // This is the genome: the entire collection of genes (the entirety of an
  // organism's hereditary information).
  genome_t genome_;
};  // class individual


// ---- Non-member functions ----
[[nodiscard]] bool operator==(const individual &, const individual &);
[[nodiscard]] unsigned distance(const individual &, const individual &);
[[nodiscard]] std::size_t active_slots(const individual &) noexcept;

// Recombination operators.
[[nodiscard]] individual crossover(const problem &,
                                   const individual &, const individual &);

// Visualization/output functions.
std::ostream &graphviz(std::ostream &, const individual &);
std::ostream &in_line(std::ostream &, const individual &);
std::ostream &operator<<(std::ostream &, const individual &);

#include "kernel/hga/individual.tcc"

}  // namespace ultra::hga

#endif  // include guard
