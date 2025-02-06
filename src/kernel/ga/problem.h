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

#if !defined(ULTRA_GA_PROBLEM_H)
#define      ULTRA_GA_PROBLEM_H

#include "kernel/problem.h"
#include "kernel/ga/primitive.h"

namespace ultra::ga
{

///
/// Provides a GA-specific interface to the generic `problem` class.
///
/// The class is a facade that provides a simpler interface to represent
/// GA-specific problems.
///
class problem : public ultra::problem
{
public:
  // ---- Constructors ----
  problem() = default;
  problem(std::size_t, const interval<int> &);
  explicit problem(const std::vector<interval<int>> &);

  [[nodiscard]] std::size_t parameters() const noexcept;

  template<symbol_set::weight_t = symbol_set::default_weight>
  integer *insert(const interval<int> &,
                  symbol::category_t = symbol::undefined_category);
};

///
/// Adds a `ga::integer` terminal to the internal symbol set.
///
/// \param[in] itval    the half open interval for the `integer` terminal
/// \param[in] category an optional category
/// \return             a raw pointer to the symbol just added (or `nullptr` in
///                     case of error)
///
template<symbol_set::weight_t W>
integer *problem::insert(const interval<int> &itval,
                         symbol::category_t category)
{
  return sset.insert<integer, W>(itval, category);
}

}  // namespace ultra::ga

#endif  // include guard
