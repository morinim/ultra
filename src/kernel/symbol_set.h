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

#if !defined(ULTRA_SYMBOL_SET_H)
#define      ULTRA_SYMBOL_SET_H

#include <memory>
#include <set>
#include <string>

#include "kernel/terminal.h"
#include "kernel/gp/function.h"

namespace ultra
{

namespace internal
{

struct w_symbol
{
  using weight_t = unsigned;

  /// This is the default weight.
  static constexpr weight_t base_weight = 100;

  explicit w_symbol(const symbol *, weight_t = base_weight);

  [[nodiscard]] bool operator==(const w_symbol &) const = default;

  const symbol *sym;

  /// Used by the symbol_set::roulette method to control the probability of
  /// selection.
  weight_t weight;
};

[[nodiscard]] bool is_terminal(const w_symbol &);
[[nodiscard]] bool is_function(const w_symbol &);

class sum_container
{
public:
  explicit sum_container(std::string);

  void insert(const w_symbol &);

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] const w_symbol &operator[](std::size_t) const;

  [[nodiscard]] auto begin();
  [[nodiscard]] auto begin() const;
  [[nodiscard]] auto end();
  [[nodiscard]] auto end() const;

  [[nodiscard]] w_symbol::weight_t sum() const;

  template<class F> void scale_weights(double, F);
  [[nodiscard]] const symbol *roulette() const;

  [[nodiscard]] bool is_valid() const;

private:
  std::vector<w_symbol> elems_ {};

  // Sum of the weights of the symbols in the container.
  w_symbol::weight_t sum_ {0};

  std::string name_;
};

// A collection is a structured-view on `symbols_` or on a subset of
// `symbols_` (e.g. only on symbols of a specific category).
class collection
{
public:
  explicit collection(std::string = "");

  [[nodiscard]] bool is_valid() const;

  void insert(const w_symbol &);

  sum_container       all {"all"};
  sum_container functions {"functions"};
  sum_container terminals {"terminals"};

private:
  std::string name_;
};

}  // namespace internal

///
/// A container for the symbols used by the engine.
///
/// Symbols are stored so as to be quickly recalled by category.
///
/// \note
/// The functions and terminals used should be powerful enough to be able to
/// represent a solution to the problem. On the other hand, it's better not
/// to use a symbol set too large (this enlarges the search space and makes
/// harder the search for a solution).
///
class symbol_set
{
public:
  // ---- Member types ----
  using weight_t = internal::w_symbol::weight_t;
  static constexpr weight_t default_weight = internal::w_symbol::base_weight;

  // ---- Constructor ----
  symbol_set() = default;

  // ---- Modifiers ----
  void clear();

  symbol *insert(std::unique_ptr<symbol>, weight_t = default_weight);
  template<Symbol S, weight_t = default_weight, class ...Args>
  std::add_pointer_t<S> insert(Args &&...);

  // ---- Capacity ----
  [[nodiscard]] symbol::category_t categories() const noexcept;
  [[nodiscard]] std::size_t functions(
    symbol::category_t = symbol::default_category) const;
  [[nodiscard]] std::size_t terminals(
    symbol::category_t = symbol::default_category) const;

  // ---- Lookup / symbol access ----
  [[nodiscard]] const symbol *roulette(
    symbol::category_t = symbol::default_category) const;
  [[nodiscard]] const function *roulette_function(
    symbol::category_t = symbol::default_category) const;
  [[nodiscard]] value_t roulette_terminal(
    symbol::category_t = symbol::default_category) const;
  [[nodiscard]] value_t roulette_terminal(
    std::size_t, symbol::category_t, weight_t = default_weight) const;
  [[nodiscard]] const symbol *roulette_free(
    symbol::category_t = symbol::default_category) const;

  [[nodiscard]] const symbol *decode(symbol::opcode_t) const;
  [[nodiscard]] const symbol *decode(const std::string &) const;

  // ---- Misc ----
  [[nodiscard]] weight_t weight(const symbol &) const;

  [[nodiscard]] std::set<symbol::category_t>
    categories_missing_terminal() const;

  [[nodiscard]] bool enough_terminals() const;
  [[nodiscard]] bool is_valid() const;

  friend std::ostream &operator<<(std::ostream &, const symbol_set &);

private:
  // This is the real, raw repository of symbols (it owns/stores the symbols).
  std::vector<std::unique_ptr<symbol>> symbols_ {};

  // The last element of the vector contains the category-agnostic view of
  // symbols:
  // - `views_.back().all.size()` is equal to the total number of symbols
  // - `views_[0].all.size()` is the number of symbols in category `0`
  std::vector<internal::collection> views_ {};
};

///
/// Adds a symbol to the symbol set.
///
/// \tparam    S    symbol to be added
/// \tparam    W    weight associated to `S`
/// \param[in] args arguments used to build `S`
/// \return         a raw pointer to the symbol just added (or `nullptr` in
///                 case of error)
///
/// Insert a symbol in the symbol set without the user having to allocate
/// memory.
///
/// \note
/// Only partially replaces the `insert(std::unique_ptr)` method (e.g. building
/// from factory).
///
template<Symbol S, symbol_set::weight_t W, class ...Args>
std::add_pointer_t<S> symbol_set::insert(Args &&... args)
{
  return static_cast<std::add_pointer_t<S>>(
    insert(std::make_unique<S>(std::forward<Args>(args)...), W));
}

std::ostream &operator<<(std::ostream &, const symbol_set &);

}  // namespace ultra

#endif  // include guard
