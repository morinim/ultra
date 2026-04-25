/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_REFINER_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_REFINER_TCC)
#define      ULTRA_REFINER_TCC

///
/// Applies a refinement backend to `ind`.
///
/// `backend` receives `(ind, eva, params)` and is expected to update `ind`.
///
template<Evaluator E, class Backend>
std::optional<evaluator_fitness_t<E>> refiner::optimise(
  evaluator_individual_t<E> &ind, const E &eva, Backend &&backend) const
{
  return std::forward<Backend>(backend)(ind, eva, params_);
}

#endif  // include guard
