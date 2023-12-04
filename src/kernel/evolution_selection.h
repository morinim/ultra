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

#if !defined(ULTRA_EVOLUTION_SELECTION_H)
#define      ULTRA_EVOLUTION_SELECTION_H

#include <set>

#include "kernel/evaluator.h"
#include "kernel/population.h"

namespace ultra::selection
{

///
/// The selection strategy (tournament, fitness proportional...).
///
template<Evaluator E>
class strategy
{
public:
  explicit strategy(E &, const environment &);

protected:
  E &eva_;
  const environment &env_;
};

///
/// Tournament selection is a method of selecting an individual from a
/// population of individuals. It involves running several *tournaments*
/// among a few individuals chosen *at random* from the population. The
/// winner of each tournament (the one with the best fitness) is selected
/// for crossover.
///
/// Selection pressure is easily adjusted by changing the tournament size.
/// If the tournament size is larger, weak individuals have a smaller chance
/// to be selected.
/// A 1-way tournament selection is equivalent to random selection.
///
/// Tournament selection has several benefits: it's efficient to code, works
/// on parallel architectures and allows the selection pressure to be easily
/// adjusted.
///
/// The tournament selection algorithm we implemented was modified so that
/// instead of having only one winner (parent) in each tournament, we select
/// `n` winners from each tournament based on the top `n` fitness values in the
/// tournament.
///
template<Evaluator E>
class tournament : public strategy<E>
{
public:
  using tournament::strategy::strategy;

  template<Population P>
  [[nodiscard]] std::vector<typename P::value_type> operator()(const P &);
};

template<Evaluator E> tournament(E &, const environment &) -> tournament<E>;

#include "kernel/evolution_selection.tcc"

}  // namespace ultra::selection

#endif  // include guard