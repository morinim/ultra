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

#if !defined(ULTRA_HGA_PROBLEM_H)
#define      ULTRA_HGA_PROBLEM_H

#include "kernel/problem.h"
#include "kernel/hga/primitive.h"

namespace ultra::hga
{

///
/// Provides a heterogeneous GA-specific interface to the generic `problem`
/// class.
///
/// The class is a facade that provides a simpler interface to represent
/// HGA-specific problems.
///
class problem : public ultra::problem
{
public:
  // ---- Constructors ----
  problem() = default;

  [[nodiscard]] std::size_t parameters() const noexcept;

  template<Terminal T, symbol_set::weight_t = symbol_set::default_weight,
           class ...Args>
  std::add_pointer_t<T> insert(Args &&...);
};

///
/// Adds a terminal to the internal symbol set.
///
/// \tparam    T    terminal to be added
/// \param[in] args arguments used to build `T`
/// \return         a raw pointer to the symbol just added (or `nullptr` in
///                 case of error)
///
template<Terminal T, symbol_set::weight_t W, class ...Args>
std::add_pointer_t<T> problem::insert(Args &&... args)
{
  return sset.insert<T, W>(std::forward<Args>(args)...);
}

}  // namespace ultra::hga

#endif  // include guard
