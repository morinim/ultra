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

#include <functional>
#include "kernel/random.h"

#include "kernel/individual.h"
#include "kernel/fitness.h"

namespace ultra
{

template<class> struct closure_arg;

template<class F>  // overloaded operator () (e.g. std::function)
struct closure_arg
  : closure_arg<decltype(&std::remove_reference_t<F>::operator())>
{
};

template<class R, class Arg>  // free functions
struct closure_arg<R(Arg)>
{
  using type = Arg;
};

template<class R, class Arg>  // function pointers
struct closure_arg<R(*)(Arg)> : closure_arg<R(Arg)>
{
};

template<class R, class C, class Arg>  // member functions
struct closure_arg<R(C::*)(Arg)> : closure_arg<R(Arg)>
{
};

template<class R, class C, class Arg>  // const member functions (and lambdas)
struct closure_arg<R(C::*)(Arg) const> : closure_arg<R(C::*)(Arg)>
{
};

template<class F> using closure_arg_t = typename closure_arg<F>::type;

template<class F,
         class C = std::remove_cvref_t<F>,
         class I = std::remove_cvref_t<closure_arg_t<C>>>
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

  [[nodiscard]] double operator()(const I &) const;

private:
  mutable std::vector<I> buffer_ {};
  const test_evaluator_type et_;
};

#include "kernel/evaluator.tcc"

}  // namespace ultra

#endif  // include guard
