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

#include "kernel/individual.h"
#include "kernel/interval.h"
#include "kernel/problem.h"

#include <functional>

namespace ultra::de
{

///
/// An individual optimized for differential evolution.
///
/// \see
/// - https://github.com/morinim/ultra/wiki/bibliography#4
/// - https://github.com/morinim/ultra/wiki/bibliography#5
///
class individual final : public ultra::individual
{
public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const ultra::problem &);

  // ---- Member types ----
  using genome_t       = std::vector<double>;
  using const_iterator = genome_t::const_iterator;
  using iterator       = genome_t::iterator;
  using value_type     = genome_t::value_type;

  // ---- Iterators ----
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  // ---- Element access ----
  [[nodiscard]] value_type operator[](std::size_t) const;

  template<class F> requires std::invocable<F &, individual::value_type &>
  void apply(std::size_t, std::size_t, F &&);
  template<class F> requires std::invocable<F &, individual::value_type &>
  void apply(F &&);

  // ---- Recombination operators ----
  [[nodiscard]] individual crossover(double, const interval<double> &,
                                     const individual &, const individual &,
                                     const individual &) const;

  // ---- Capacity ----
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t parameters() const noexcept;

  // ---- Misc ----
  [[nodiscard]] operator std::vector<value_type>() const noexcept;
  individual &operator=(const std::vector<value_type> &);

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] bool is_valid() const;

private:
  // ---- Private support methods ----
  [[nodiscard]] hash_t hash() const;

  // Serialization.
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  // ---- Private data members ----

  // This is the genome: the entire collection of genes (the entirety of an
  // organism's hereditary information).
  genome_t genome_;
};  // class individual


[[nodiscard]] bool operator==(const individual &, const individual &);
[[nodiscard]] double distance(const individual &, const individual &);
[[nodiscard]] std::size_t active_slots(const individual &) noexcept;

// Visualization/output functions.
std::ostream &graphviz(std::ostream &, const individual &);
std::ostream &in_line(std::ostream &, const individual &);
std::ostream &operator<<(std::ostream &, const individual &);

#include "kernel/de/individual.tcc"

}  // namespace ultra::de

#endif  // include guard
