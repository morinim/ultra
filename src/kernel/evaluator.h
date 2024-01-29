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

#if !defined(ULTRA_EVALUATOR_H)
#define      ULTRA_EVALUATOR_H

#include <functional>
#include <thread>

#include "kernel/individual.h"
#include "kernel/fitness.h"
#include "kernel/random.h"

#include "utility/misc.h"

namespace ultra
{

template<class F,
         class I = closure_arg_t<F>>
concept Evaluator =
  Individual<I> && std::invocable<F, I> && Fitness<std::invoke_result_t<F, I>>;


enum class test_evaluator_type {distinct, fixed, random};

///
/// A fitness function used for debug purpose.
///
/// It can be:
/// - a unique fitness (`test_evaluator_type::distinct`). Every individual has
///   his own (time invariant) fitness;
/// - a random (time invariant) fitness (`test_evaluator_type::random`);
/// - a constant fitness (`test_evaluator_type::fixed`). Same fitness for the
///   entire population.
///
template<Individual I>
class test_evaluator
{
public:
  explicit test_evaluator(test_evaluator_type = test_evaluator_type::random);

  void delay(std::chrono::milliseconds);

  [[nodiscard]] double operator()(const I &) const;

private:
  mutable std::vector<I> buffer_ {};
  mutable std::mutex mutex_ {};

  const test_evaluator_type et_;

  std::chrono::milliseconds delay_ {0};
};

#include "kernel/evaluator.tcc"

}  // namespace ultra

#endif  // include guard
