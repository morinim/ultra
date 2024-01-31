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

#if !defined(ULTRA_EVOLUTION_STRATEGY_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_STRATEGY_TCC)
#define      ULTRA_EVOLUTION_STRATEGY_TCC

template<Individual I, Fitness F>
evolution_strategy<I, F>::evolution_strategy(population_t &pop,
                                             summary<I, F> &sum)
  : sum_(sum), pop_(pop)
{
}

template<Evaluator E, Individual I, Fitness F>
alps_es<E, I, F>::alps_es(population_t &pop,
                          typename population_t::layer_iter l,
                          E &eva,
                          summary<I, F> &sum)
  : evolution_strategy<I, F>(pop, sum),
    sel_pop_(alps::selection_layers(pop, l)),
    rep_pop_(alps::replacement_layers(pop, l)),
    select_(eva, pop.problem().env),
    recombine_(eva, pop.problem(), sum),
    replace_(eva, pop.problem().env, sum)
{
  static_assert(std::is_same_v<I, closure_arg_t<E>>);
  static_assert(std::is_same_v<F, closure_return_t<E>>);
}

template<Evaluator E, Individual I, Fitness F>
void alps_es<E, I, F>::operator()()
{
  Ensures(!sel_pop_.empty());
  Ensures(!rep_pop_.empty());
  Ensures(&sel_pop_.front().get() == &rep_pop_.front().get());

  const auto parents(select_(sel_pop_));
  const auto offspring(recombine_(parents));
  this->replace_(rep_pop_, offspring[0]);
}

#endif  // include guard
