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

#if !defined(ULTRA_NUMERICAL_OPTIMISER_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_NUMERICAL_OPTIMISER_TCC)
#define      ULTRA_NUMERICAL_OPTIMISER_TCC

///
/// Refines the tunable scalar parameters of `ind` by delegating to
/// `backend`.
///
/// `backend` receives `(ind, eva, params)` and is expected to update `ind`
/// with the numerically improved decision vector.
///
template<NumericalOptimisable I, Evaluator E, class Backend>
void numerical_optimiser::optimise(I &ind, const E &eva,
                                   Backend &&backend) const
{
  std::forward<Backend>(backend)(ind, eva, params_);
}

#endif  // include guard
