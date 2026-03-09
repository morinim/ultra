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
#include "kernel/evolution_selection.h"
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
  using individual_t = evaluator_individual_t<E>;
  using fitness_t    = evaluator_fitness_t<E>;
  using status_t     = evolution_status<individual_t, fitness_t>;

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
/// \see https://github.com/morinim/ultra/wiki/bibliography#24
///
template<Evaluator E>
class tournament : public strategy<E>
{
public:
  using strategy<E>::strategy;
  using typename strategy<E>::individual_t;
  using typename strategy<E>::status_t;

  template<Population P>
  bool operator()(P &, const individual_t &, status_t &) const;
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
  using strategy<E>::strategy;
  using typename strategy<E>::individual_t;
  using typename strategy<E>::status_t;

  template<PopulationWithMutex P>
  void operator()(alps_layer_pair<P>, const individual_t &, status_t &) const;

  template<PopulationWithMutex P>
  void try_promote_individuals(const P &, P &) const;

private:
  template<PopulationWithMutex P>
  bool try_add_to_layer(alps_layer_pair<P>, const individual_t &) const;
  template<PopulationWithMutex P>
  bool try_add_to_layer(P &, const individual_t &) const;
};

template<Evaluator E> alps(E &, const parameters &) -> alps<E>;

template<Evaluator E>
class de : public strategy<E>
{
public:
  using strategy<E>::strategy;
  using typename strategy<E>::individual_t;
  using typename strategy<E>::status_t;

  bool operator()(individual_t &, const individual_t &, status_t &) const;
};

template<Evaluator E> de(E &, const parameters &) -> de<E>;

#include "kernel/evolution_replacement.tcc"

}  // namespace ultra::replacement

#endif  // include guard
