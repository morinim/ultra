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
                                             evolution_status<I, F> &status)
  : status_(status), pop_(pop)
{
}

template<Evaluator E, Individual I, Fitness F>
alps_es<E, I, F>::alps_es(population_t &pop,
                          E &eva,
                          evolution_status<I, F> &status)
  : evolution_strategy<I, F>(pop, status),
    select_(eva, pop.problem().env),
    recombine_(eva, pop.problem(), status),
    replace_(eva, pop.problem().env, status)
{
  static_assert(std::is_same_v<I, closure_arg_t<E>>);
  static_assert(std::is_same_v<F, closure_return_t<E>>);
}

template<Evaluator E, Individual I, Fitness F>
auto alps_es<E, I, F>::operations(typename population_t::layer_iter l) const
{
  return
    [this,
     sel_pop = alps::selection_layers(this->pop_, l),
     rep_pop = alps::replacement_layers(this->pop_, l)]()
    {
      Ensures(!sel_pop.empty());
      Ensures(!rep_pop.empty());
      Ensures(&sel_pop.front().get() == &rep_pop.front().get());

      const auto parents(this->select_(sel_pop));
      const auto offspring(this->recombine_(parents).front());
      this->replace_(rep_pop, offspring);
    };
}

template<Evaluator E, Individual I, Fitness F>
std_es<E, I, F>::std_es(population_t &pop,
                        E &eva,
                        evolution_status<I, F> &status)
  : evolution_strategy<I, F>(pop, status),
    select_(eva, pop.problem().env),
    recombine_(eva, pop.problem(), status),
    replace_(eva, pop.problem().env, status)
{
  static_assert(std::is_same_v<I, closure_arg_t<E>>);
  static_assert(std::is_same_v<F, closure_return_t<E>>);
}

template<Evaluator E, Individual I, Fitness F>
auto std_es<E, I, F>::operations(typename population_t::layer_iter l) const
{
  Expects(this->pop_.layers());
  Expects(iterator_of(l, this->pop_.range_of_layers()));

  return
    [this, &pop_layer = *l]()
    {
      Ensures(!pop_layer.empty());

      const auto parents(this->select_(pop_layer));
      const auto offspring(this->recombine_(parents).front());
      this->replace_(pop_layer, offspring);
    };
}

#endif  // include guard
