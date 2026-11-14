/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_VECTOR_INDIVIDUAL_H)
#define      ULTRA_VECTOR_INDIVIDUAL_H

#include "kernel/individual.h"
#include "kernel/problem.h"
#include "utility/log.h"
#include "utility/misc.h"

#include <concepts>
#include <functional>
#include <utility>

namespace ultra
{

///
/// Provides a shared base for individuals with genomes stored in a vector.
///
/// \tparam T gene value type
///
/// Shares genome access, signature maintenance and serialisation. Derived
/// classes define evolutionary operators.
///
template<class T>
class vector_individual : public individual
{
public:
  static_assert(std::same_as<T, D_INT>
                || std::same_as<T, D_DOUBLE>
                || std::same_as<T, value_t>,
                "Unsupported gene type");

  // ---- Member types ----
  using genome_t       = std::vector<T>;
  using const_iterator = genome_t::const_iterator;
  using iterator       = genome_t::iterator;
  using value_type     = genome_t::value_type;

  // ---- Iterators ----
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  // ---- Element access ----
  [[nodiscard]] const value_type &operator[](std::size_t) const;

  // ---- Modifiers ----
  vector_individual &operator=(const std::vector<T> &);

  // ---- Capacity ----
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t parameters() const noexcept;

  // ---- Misc ----
  [[nodiscard]] operator const std::vector<T> &() const noexcept;

  [[nodiscard]] bool is_valid() const;

protected:
  // ---- Constructors ----
  vector_individual() = default;
  explicit vector_individual(std::size_t);
  explicit vector_individual(const ultra::problem &);

  // ---- Modifiers ----
  template<class F>
  requires std::invocable<F &, T &>
           && std::same_as<std::invoke_result_t<F &, T &>, void>
  void apply_each(std::size_t, std::size_t, F &&);

  template<class F>
  requires std::invocable<F &, T &>
           && std::same_as<std::invoke_result_t<F &, T &>, void>
  void apply_each(F &&);

  // ---- Support methods ----
  [[nodiscard]] hash_t hash() const override;

  // This is the genome: the entire collection of genes (the entirety of an
  // organism's hereditary information).
  genome_t genome_;

private:
  // ---- Serialisation ----
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;
};  // class vector_individual

// ---- Non-member functions ----
template<class T> [[nodiscard]] bool operator==(
  const vector_individual<T> &, const vector_individual<T> &) noexcept;
template<class T> [[nodiscard]] std::size_t active_slots(
  const vector_individual<T> &) noexcept;

#include "kernel/vector_individual.tcc"

}  // namespace ultra

#endif  // include guard
