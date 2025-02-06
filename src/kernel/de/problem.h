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

#if !defined(ULTRA_DE_PROBLEM_H)
#define      ULTRA_DE_PROBLEM_H

#include "kernel/problem.h"
#include "kernel/de/primitive.h"

namespace ultra::de
{

///
/// Provides a DE-specific interface to the generic `problem` class.
///
/// The class is a facade that provides a simpler interface to represent
/// DE-specific problems.
///
class problem : public ultra::problem
{
public:
  // ---- Constructors ----
  problem() = default;
  problem(std::size_t, const interval<double> &);
  explicit problem(const std::vector<interval<double>> &);

  [[nodiscard]] std::size_t parameters() const noexcept;

  template<symbol_set::weight_t = symbol_set::default_weight>
  real *insert(const interval<double> &,
               symbol::category_t = symbol::undefined_category);
};

///
/// Adds a `de::real` terminal to the internal symbol set.
///
/// \param[in] itval    the half open interval for the `real` terminal
/// \param[in] category an optional category
/// \return             a raw pointer to the symbol just added (or `nullptr` in
///                     case of error)
///
template<symbol_set::weight_t W>
real *problem::insert(const interval<double> &itval,
                      symbol::category_t category)
{
  return sset.insert<real, W>(itval, category);
}

}  // namespace ultra::de

#endif  // include guard
