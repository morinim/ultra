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

#include "kernel/evaluator.h"
#include "kernel/problem.h"
#include "kernel/de/search.h"
#include "kernel/gp/individual.h"

namespace ultra
{

namespace gp
{

///
/// Checks whether a type supports decision vector extraction.
///
template<class T>
concept DecisionVectorExtractable = requires(const T &t)
{
  { extract_decision_vector(t) } -> std::same_as<gp::decision_vector>;
};

class numerical_optimiser
{
public:
  explicit numerical_optimiser(const problem &p);

  template<Evaluator E> void optimise(gp::individual &, const E &) const;

private:
  const parameters::numerical_optimisation_parameters params_;
};

#include "numerical_optimiser.tcc"

}  // namespace ultra::gp

}  // namespace ultra

#endif  // include guard
