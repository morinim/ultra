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

#include "utility/log.h"
#include "utility/misc.h"

namespace ultra
{

template<class F, class I = closure_arg_t<F>>
concept Evaluator =
  Individual<I> && std::invocable<F, I> && Fitness<std::invoke_result_t<F, I>>;

template<Evaluator E> using evaluator_individual_t = closure_arg_t<E>;
template<Evaluator E> using evaluator_fitness_t = closure_return_t<E>;

// Usually persistance of simple evaluators isn't required. Unusual evaluators
// have to implement load/save.
template<Evaluator E> bool load_eva(std::istream &, E *) { return true; }
template<Evaluator E> bool save_eva(std::ostream &, const E &) { return true; }

enum class evaluation_type {standard, fast};

enum class test_evaluator_type {realistic, fixed, random, age};

///
/// A fitness function used for debug purpose.
///
template<Individual I>
class test_evaluator
{
public:
  explicit test_evaluator(test_evaluator_type = test_evaluator_type::random);

  void delay(std::chrono::milliseconds);

  [[nodiscard]] double operator()(const I &) const;

private:
  const test_evaluator_type et_;

  std::chrono::milliseconds delay_ {0};
};

#include "kernel/evaluator.tcc"

}  // namespace ultra

#endif  // include guard
