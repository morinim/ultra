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
template<Evaluator E>
class strategy
{
public:
  strategy(E &, const problem &);

protected:
  E &eva_;
  const problem &prob_;
};

///
/// This class defines the program skeleton of a standard genetic
/// programming crossover plus mutation operation. It's a template method
/// design pattern: one or more of the algorithm steps can be overriden
/// by subclasses to allow differing behaviours while ensuring that the
/// overarching algorithm is still followed.
///
template<Evaluator E>
class base : public strategy<E>
{
public:
  using base::strategy::strategy;

  template<RandomAccessIndividuals R>
  [[nodiscard]] std::ranges::range_value_t<R> operator()(const R &) const;
};

template<Evaluator E> base(E &, const problem &) -> base<E>;

///
/// This is based on the differential evolution four members crossover.
///
class de
{
public:
  explicit de(const problem &);

  template<RandomAccessIndividuals R>
  [[nodiscard]] std::ranges::range_value_t<R> operator()(const R &) const;

private:
  const problem &prob_;
};

#include "kernel/evolution_recombination.tcc"

}  // namespace ultra::recombination

#endif  // include guard
