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
evolution_strategy<I, F>::evolution_strategy(
  population_t &pop, const evolution_status<I, F> &status)
  : starting_status_(status), pop_(pop)
{
}

template<Evaluator E, Individual I, Fitness F>
alps_es<E, I, F>::alps_es(population_t &pop,
                          E &eva,
                          const evolution_status<I, F> &status)
  : evolution_strategy<I, F>(pop, status),
    select_(eva, pop.problem().env),
    recombine_(eva, pop.problem()),
    replace_(eva, pop.problem().env)
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
     rep_pop = alps::replacement_layers(this->pop_, l),
     status = this->starting_status_]() mutable
    {
      Ensures(!sel_pop.empty());
      Ensures(!rep_pop.empty());
      Ensures(&sel_pop.front().get() == &rep_pop.front().get());

      const auto parents(this->select_(sel_pop));
      const auto offspring(this->recombine_(parents).front());
      this->replace_(rep_pop, offspring, status);
    };
}

///
/// Increments population's age and checks if it's time to add a new layer.
///
template<Evaluator E, Individual I, Fitness F>
void alps_es<E, I, F>::after_generation(const analyzer<I, F> &az)
{
  auto &pop(this->pop_);
  const auto &env(pop.problem().env);

  pop.inc_age();

  const auto layers(pop.range_of_layers());

  if (pop.layers() > 1)
  {
    std::size_t l_index(1);

    for (auto layer(std::next(layers.begin()));
         layer != layers.end();
         ++l_index)
    {
      if (almost_equal(az.fit_dist(l_index - 1).mean(),
                       az.fit_dist(l_index).mean()))
        layer = pop.erase(layer);
      else
      {
        const auto sd(az.fit_dist(l_index).standard_deviation());
        const bool converged(issmall(sd));

        if (converged)
          layer->allowed(std::max(env.min_individuals, layer->size() / 2));
        else
          layer->allowed(env.individuals);

        ++layer;
      }
    }
  }

  // Code executed every `age_gap` interval.
  if (const auto generation(this->starting_status_);
      generation && generation % env.alps.age_gap == 0)
  {
    if (pop.layers() < env.layers
        || az.age_dist(layers - 1).mean() > env.alps.max_age(layers))
      pop.add_layer();
    else
    {
      this->replacement.try_move_up_layer(0);
      pop.front.init();
    }
  }
}

///
/// \param[out] env environemnt
/// \return         a strategy-specific environment
///
/// \remark
/// ALPS requires more than one layer.
///
template<Evaluator E, Individual I, Fitness F>
environment alps_es<E, I, F>::shape(environment env)
{
  env.population.layers = 4;
  return env;
}

template<Evaluator E, Individual I, Fitness F>
std_es<E, I, F>::std_es(population_t &pop,
                        E &eva,
                        const evolution_status<I, F> &status)
  : evolution_strategy<I, F>(pop, status),
    select_(eva, pop.problem().env),
    recombine_(eva, pop.problem()),
    replace_(eva, pop.problem().env)
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
    [this, &pop_layer = *l, status = this->starting_status_]() mutable
    {
      Ensures(!pop_layer.empty());

      const auto parents(this->select_(pop_layer));
      const auto offspring(this->recombine_(parents).front());
      this->replace_(pop_layer, offspring, status);
    };
}

#endif  // include guard
