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

#if !defined(ULTRA_PROBLEM_H)
#define      ULTRA_PROBLEM_H

#include "kernel/parameters.h"
#include "kernel/symbol_set.h"

namespace ultra
{

///
/// Aggregates the problem-related data needed by an evolutionary program.
///
struct problem
{
  template<Symbol S, symbol_set::weight_t = symbol_set::default_weight,
           class ...Args>
  requires std::constructible_from<S, Args...>
  std::add_pointer_t<S> insert(Args &&...);

  template<std::derived_from<function> F,
           symbol_set::weight_t W = symbol_set::default_weight,
           std::convertible_to<symbol::category_t> C = symbol::category_t>
  std::add_pointer_t<F> insert(symbol::category_t, std::initializer_list<C>);

  template<std::derived_from<function> F,
           symbol_set::weight_t W = symbol_set::default_weight,
           std::convertible_to<symbol::category_t> C = symbol::category_t>
  std::add_pointer_t<F> insert(std::initializer_list<C>);

  [[nodiscard]] virtual bool is_valid() const;

  parameters params {};
  symbol_set sset {};
};

///
/// Adds a symbol to the internal symbol set.
///
/// \tparam    S    symbol to be added
/// \param[in] args arguments used to build `S`
/// \return         a raw pointer to the symbol just added (or `nullptr` in
///                 case of error)
///
template<Symbol S, symbol_set::weight_t W, class ...Args>
requires std::constructible_from<S, Args...>
std::add_pointer_t<S> problem::insert(Args &&... args)
{
  return sset.insert<S, W>(std::forward<Args>(args)...);
}

template<std::derived_from<function> F, symbol_set::weight_t W,
         std::convertible_to<symbol::category_t> C>
std::add_pointer_t<F> problem::insert(symbol::category_t ret,
                                      std::initializer_list<C> pt)
{
  return sset.insert<F, W>(ret, pt);
}

template<std::derived_from<function> F, symbol_set::weight_t W,
         std::convertible_to<symbol::category_t> C>
std::add_pointer_t<F> problem::insert(std::initializer_list<C> pt)
{
  return sset.insert<F, W>(pt);
}

}  // namespace ultra

#endif  // include guard
