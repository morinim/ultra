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

#if !defined(ULTRA_EVOLUTION_REPLACEMENT_H)
#define      ULTRA_EVOLUTION_REPLACEMENT_H

#include "kernel/alps.h"
#include "kernel/evaluator.h"
#include "kernel/evolution_status.h"
#include "kernel/linear_population.h"

namespace ultra::replacement
{
///
/// The replacement strategy (random, tournament...) for the
/// ultra::evolution_strategy class.
///
template<Evaluator E>
class strategy
{
public:
  strategy(E &, const parameters &);

protected:
  E &eva_;
  const parameters &params_;
};

///
/// Tournament based replacement scheme (aka kill tournament).
///
/// This strategy select an individual for replacement by kill tournament:
/// pick a number of individuals at random and replace the worst.
///
/// \see
/// "Replacement Strategies in Steady State Genetic Algorithms: Static
/// Environments" - Jim Smith, Frank Vavak.
///
template<Evaluator E>
class tournament : public strategy<E>
{
public:
  using tournament::strategy::strategy;

  template<Population P>
  void operator()(
    P &, const evaluator_individual_t<E> &,
    evolution_status<evaluator_individual_t<E>,
                     evaluator_fitness_t<E>> &) const;
};

template<Evaluator E> tournament(E &, const parameters &) -> tournament<E>;

///
/// ALPS based replacement scheme.
///
/// This strategy select an individual for replacement by an ad hoc kill
/// tournament.
/// When an individual is too old for its current layer, it cannot be used to
/// generate new individuals for that layer and eventually is removed from the
/// layer. Optionally, an attempt can be made to move this individual up to the
/// next layer -- in which case it replaces some individual there that it's
/// better than.
///
template<Evaluator E>
class alps : public strategy<E>
{
public:
  using alps::strategy::strategy;

  template<PopulationWithMutex P, Individual I>
  void operator()(
    std::vector<std::reference_wrapper<P>>, const I &,
    evolution_status<evaluator_individual_t<E>,
                     evaluator_fitness_t<E>> &) const;

  template<SizedRandomAccessPopulation P>
  void try_move_up_layer(const P &, P &);

private:
  template<PopulationWithMutex P, Individual I>
  bool try_add_to_layer(std::vector<std::reference_wrapper<P>>,
                        const I &) const;
};

template<Evaluator E> alps(E &, const parameters &) -> alps<E>;

#include "kernel/evolution_replacement.tcc"

}  // namespace ultra::replacement

#endif  // include guard
