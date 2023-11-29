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

#if !defined(ULTRA_EVALUATOR_H)
#define      ULTRA_EVALUATOR_H

#include "kernel/individual.h"
#include "kernel/fitness.h"

namespace ultra
{

template<Individual I, class E> concept Evaluator = requires(E eva)
{
  { eva(I()) } -> std::convertible_to<Fitness>;
};

}  // namespace ultra

#endif  // include guard
