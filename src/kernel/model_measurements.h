/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_MODEL_MEASUREMENTS_H)
#define      ULTRA_MODEL_MEASUREMENTS_H

#include "kernel/fitness.h"

namespace ultra
{
///
/// A collection of measurements.
///
template<Fitness F>
struct model_measurements
{
  model_measurements() = default;

  model_measurements(const F &f, double a, bool s)
    : fitness(f), accuracy(a), is_solution(s)
  {
    Expects(accuracy <= 1.0);
  }

  F        fitness {};
  double  accuracy {-1.0};
  bool is_solution {false};
};

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparision
///
/// \warning
/// This is a partial ordering relation since is somewhat based on Pareto
/// dominance.
///
/// \relates model_measurements
///
template<Fitness F>
[[nodiscard]] bool operator>=(const model_measurements<F> &lhs,
                              const model_measurements<F> &rhs)
{
  return dominating(lhs.fitness, rhs.fitness) &&
         lhs.accuracy >= rhs.accuracy;
}

}  // namespace ultra

#endif  // include guard
