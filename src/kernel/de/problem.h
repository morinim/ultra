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
  problem() = default;
  problem(std::size_t, const interval_t<double> &);
  explicit problem(const std::vector<interval_t<double>> &);

  [[nodiscard]] std::size_t parameters() const noexcept;

  template<symbol_set::weight_t = symbol_set::default_weight,
           class ...Args>
  real *insert(Args &&...);
};

///
/// Adds a symbol to the internal symbol set.
///
/// \param[in] args arguments used to build the `de::real` terminal
/// \return         a raw pointer to the symbol just added (or `nullptr` in
///                 case of error)
///
template<symbol_set::weight_t W, class ...Args>
real *problem::insert(Args &&... args)
{
  return sset.insert<real, W>(std::forward<Args>(args)...);
}

}  // namespace ultra::de

#endif  // include guard
