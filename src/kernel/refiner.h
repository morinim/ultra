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
#define      ULTRA_REFINER_H

#include "kernel/evaluator.h"
#include "kernel/problem.h"

#include <optional>

namespace ultra
{

class refiner
{
public:
  explicit refiner(const problem &p);

  template<Evaluator E, class Backend>
  std::optional<evaluator_fitness_t<E>> optimise(evaluator_individual_t<E> &,
                                                 const E &, Backend &&) const;

private:
  const parameters::refinement_parameters params_;
};

#include "refiner.tcc"

}  // namespace ultra

#endif  // include guard
