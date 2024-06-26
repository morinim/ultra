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
  std::add_pointer_t<S> insert(Args &&...);

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
std::add_pointer_t<S> problem::insert(Args &&... args)
{
  return sset.insert<S, W>(std::forward<Args>(args)...);
}

}  // namespace ultra

#endif  // include guard
