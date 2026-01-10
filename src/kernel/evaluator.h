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

///
/// Defines the requirements for a fitness evaluator.
///
/// An Evaluator is a callable object that:
/// - operates on an `Individual` type;
/// - can be invoked on a `const` instance of the evaluator;
/// - returns a value satisfying the `Fitness` concept.
///
/// Requiring const-invocability ensures that evaluators can be safely used
/// through read-only references, which is essential when they are wrapped
/// by utility classes such as `evaluator_proxy` and invoked concurrently
/// or through logically-const interfaces.
///
template<class F, class I = closure_arg_t<F>>
concept Evaluator =
  Individual<I> && std::invocable<const F &, I>
  && Fitness<std::invoke_result_t<F, I>>;

template<Evaluator E> using evaluator_individual_t = closure_arg_t<E>;
template<Evaluator E> using evaluator_fitness_t = closure_return_t<E>;

enum class evaluation_type {standard, fast};

enum class test_evaluator_type {realistic, fixed, random, age};

///
/// A configurable fitness evaluator intended for testing and debugging.
///
/// \tparam I an `Individual` type
///
/// test_evaluator provides several simple, deterministic or stochastic fitness
/// strategies that are useful for:
/// - validating the behaviour of evolutionary operators;
/// - benchmarking infrastructure components (e.g. parallel evaluation,
///   caching, scheduling);
/// - debugging population dynamics without relying on a real problem domain.
///
/// The evaluator can optionally introduce a fixed delay for each evaluation,
/// allowing simulation of expensive fitness computations.
///
/// \note
/// This evaluator is primarily intended for testing and debugging purposes.
/// It makes no guarantees about fitness meaningfulness or collision
/// resistance.
///
/// \remark
/// The class is not thread-safe if `delay` is called concurrently with
/// evaluation. Concurrent evaluation without modifying the delay is safe
/// provided that the underlying random generator is thread-safe.
///
template<Individual I>
class test_evaluator
{
public:
  explicit test_evaluator(test_evaluator_type = test_evaluator_type::random);

  void delay(std::chrono::milliseconds) noexcept;

  [[nodiscard]] double operator()(const I &) const;

private:
  const test_evaluator_type et_;

  std::chrono::milliseconds delay_ {0};
};

#include "kernel/evaluator.tcc"

}  // namespace ultra

#endif  // include guard
