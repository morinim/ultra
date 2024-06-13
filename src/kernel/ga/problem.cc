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

#include "kernel/ga/problem.h"

namespace ultra::ga
{

///
/// Sets up a GA problem for which a solution has the given number of
/// (uniform, same range) parameters.
///
/// \param[in] nparam   number of parameters (aka genes in the chromosome)
/// \param[in] interval a half-open interval (the value of each parameter falls
///                     within this range)
///
/// The typical solution of a combinatorial problem can often be represented as
/// a sequence of integers in a given range (and this is the *raison d'etre* of
/// this constructor).
///
problem::problem(std::size_t nparam, const interval_t<int> &interval)
  : ultra::problem()
{
  Expects(parameters() == 0);

  while (nparam--)
    insert(interval);
}

///
/// Sets up a GA problem for which a solution has the given number of
/// (uniform but **not** same range) parameters.
///
/// \param[in] intervals a sequence of half-open intervals (one for each
///                      parameter)
///
/// This is a more flexible form of the other constructor. Each parameter has
/// its own range.
///
problem::problem(const std::vector<interval_t<int>> &intervals)
  : ultra::problem()
{
  Expects(parameters() == 0);

  for (const auto &interval : intervals)
    insert(interval);
}

}  // namespace vita
