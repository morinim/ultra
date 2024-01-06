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

#if !defined(ULTRA_EVOLUTION_RECOMBINATION_H)
#define      ULTRA_EVOLUTION_RECOMBINATION_H

#include "kernel/evaluator.h"
#include "kernel/evolution_summary.h"
#include "kernel/fitness.h"
#include "kernel/individual.h"
#include "kernel/population.h"
#include "kernel/problem.h"

namespace ultra::recombination
{

///
/// The operation strategy (crossover, recombination, mutation...) adopted in
/// the evolution class.
///
/// A recombination acts upon sets of individuals to generate offspring
/// (this definition generalizes the traditional mutation and crossover
/// operators).
///
/// Operator application is atomic from the point of view of the
/// evolutionary algorithm and every recombination is applied to a well
/// defined list of individuals, without dependencies upon past history.
///
/// In the strategy design pattern, this class is the strategy interface and
/// ultra::evolution is the context.
///
/// This is an abstract class: introduction of new operators or redefinition
/// of existing ones is obtained implementing recombination::strategy.
///
/// \see
/// * <http://en.wikipedia.org/wiki/Strategy_pattern>
///
template<Individual I, Fitness F>
class strategy
{
public:
  strategy(const problem &, summary<I, F> *);

protected:
  const problem &prob_;
  summary<I, F> &stats_;
};

///
/// This class defines the program skeleton of a standard genetic
/// programming crossover plus mutation operation. It's a template method
/// design pattern: one or more of the algorithm steps can be overriden
/// by subclasses to allow differing behaviours while ensuring that the
/// overarching algorithm is still followed.
///
template<Individual I, Fitness F>
class base : public strategy<I, F>
{
public:
  using base::strategy::strategy;

  template<SizedRangeOfIndividuals R, Evaluator E>
  [[nodiscard]] std::vector<std::ranges::range_value_t<R>>
  operator()(const R &, E &) const;
};

template<Individual I, Fitness F> base(const problem &, summary<I, F> *)
  -> base<I, F>;

#include "kernel/evolution_recombination.tcc"

}  // namespace ultra::recombination

#endif  // include guard
