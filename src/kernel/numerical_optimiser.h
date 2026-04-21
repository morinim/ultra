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
#define      ULTRA_NUMERICAL_OPTIMISER_H

#include "kernel/decision_vector.h"
#include "kernel/evaluator.h"
#include "kernel/problem.h"
#include "kernel/de/search.h"

namespace ultra
{

/// Concrete decision-vector type associated with `I`.
template<class I> using decision_vector_t =
  std::remove_cvref_t<
    decltype(extract_decision_vector(std::declval<const I &>()))>;

/// Checks whether a type supports numerical optimisation.
template<class I>
concept NumericalOptimisable =
  Individual<I>
  && std::copy_constructible<I>
  && DecisionVectorExtractable<I>
  && requires(I &ind, const decision_vector_t<I> &dv)
     {
       ind.apply_decision_vector(dv);
     };

class numerical_optimiser
{
public:
  explicit numerical_optimiser(const problem &p);

  template<NumericalOptimisable I, Evaluator E>
  void optimise(I &, const E &) const;

private:
  const parameters::numerical_optimisation_parameters params_;
};

#include "numerical_optimiser.tcc"

}  // namespace ultra

#endif  // include guard
