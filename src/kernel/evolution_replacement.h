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
#include "kernel/evolution_summary.h"
#include "kernel/population.h"

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
  strategy(E &, const environment &,
           summary<closure_arg_t<E>, closure_return_t<E>> *);

protected:
  E &eva_;
  const environment &env_;
  summary<closure_arg_t<E>, closure_return_t<E>> &stats_;
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

  template<Population P, SizedRangeOfIndividuals R>
  void operator()(P &, const R &) const;
};

template<Evaluator E> tournament(
  E &, const environment &, summary<closure_arg_t<E>, closure_return_t<E>> *)
  -> tournament<E>;

/*
///
/// ALPS based replacement scheme.
///
/// \tparam T type of program (individual/team)
///
/// This strategy select an individual for replacement by an ad hoc kill
/// tournament.
/// When an individual is too old for its current layer, it cannot be used
/// to generate new individuals for that layer and eventually is removed
/// from the layer. Optionally, an attempt can be made to move this
/// individual up to the next layer -- in which case it replaces some
/// individual there that it is better than.
///
/// \see
/// "Replacement Strategies in Steady State Genetic Algorithms: Static
/// Environments" - Jim Smith, Frank Vavak.
///
template<class T>
class alps : public strategy<T>
{
public:
  using alps::strategy::strategy;

  void run(const typename strategy<T>::parents_t &,
           const typename strategy<T>::offspring_t &, summary<T> *);

  void try_move_up_layer(unsigned);

private:
  [[nodiscard]] unsigned allowed_age(unsigned) const;
  bool try_add_to_layer(unsigned, const T &);
};
*/
#include "kernel/evolution_replacement.tcc"

}  // namespace ultra::replacement

#endif  // include guard
