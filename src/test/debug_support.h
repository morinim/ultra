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

#include "kernel/layered_population.h"

#if !defined(ULTRA_DEBUG_SUPPORT_H)
#define      ULTRA_DEBUG_SUPPORT_H

namespace ultra::debug
{

///
/// Creates a random population where each individual has a different age.
///
/// \param[in] prob current problem
///
/// This function, together with an evaluator in `test_evaluator_type::age`
/// mode, is useful for debug purpose since allows to easily distinguish among
/// individuals.
///
/// \note
/// The `==` operator of an individual doesn't compare the age, explicit check
/// must be performed by the user.
///
template<Individual I>
[[nodiscard]] layered_population<I>
make_debug_population(const ultra::problem &prob)
{
  layered_population<I> pop(prob);

  for (unsigned inc(0); auto &prg : pop)
    prg.inc_age(inc++);

  return pop;
}

}  // namespace ultra::debug

#endif  // include guard
