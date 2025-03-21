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

#include "kernel/de/problem.h"

namespace ultra::de
{

///
/// Sets up a DE problem for which a solution has the given number of
/// (uniform, same range) parameters.
///
/// \param[in] nparam number of parameters (aka genes in the chromosome)
/// \param[in] itval  a half-open interval (the value of each parameter falls
///                   within this range)
///
/// The typical solution of a numerical optimization problem can often be
/// represented as a sequence of real numbers in a given range (and this is the
/// *raison d'etre* of this constructor).
///
problem::problem(std::size_t nparam, const interval<double> &itval)
  : ultra::problem()
{
  Expects(parameters() == 0);

  while (nparam--)
    insert(itval);

  Ensures(is_valid());
}

///
/// Sets up a DE problem for which a solution has the given number of
/// (uniform but **not** same range) parameters.
///
/// \param[in] intervals a sequence of half-open intervals (one for each
///                        parameter)
///
/// This is a more flexible form of the other constructor. Each parameter has
/// its own range.
///
problem::problem(const std::vector<interval<double>> &intervals)
  : ultra::problem()
{
  Expects(parameters() == 0);

  for (const auto &itval : intervals)
    insert(itval);

  Ensures(is_valid());
}

///
/// \return genome size / number of parameters / elements in the container
///
std::size_t problem::parameters() const noexcept
{
  return sset.categories();
}

}  // namespace ultra::de
