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

template<Evaluator E>
evolution_strategy<E>::evolution_strategy(const ultra::problem &prob,
                                          const E &eva)
  : eva_(eva), prob_(&prob)
{
}

template<Evaluator E>
alps_es<E>::alps_es(const problem &prob, const E &eva)
  : evolution_strategy<E>(prob, eva),
    select_(this->eva_, prob.env),
    recombine_(this->eva_, prob),
    replace_(this->eva_, prob.env)
{
}

///
/// \param[in] pop             a population. Operations are performed on
///                            sub-populations of `pop
/// \param[in] iter            iterator pointing to the main subpopulation
///                            we're going to work on. Other sub-populations
///                            involved are `std::next(subpop_iter)` and
///                            `pop.back()`
/// \param[in] starting_status the starting status (usually generated via
///                            `summary::starting_status()`)
/// \return                    a callable object encaspulating the ALPS
///                            algorithm
///
template<Evaluator E>
template<Population P>
auto alps_es<E>::operations(
  P &pop, typename P::layer_iter iter,
  const evolution_status<individual_t, fitness_t> &starting_status) const
{
  Expects(pop.layers());
  Expects(iterator_of(iter, pop.range_of_layers()));

  return
    [this,
     sel_pop = alps::selection_layers(pop, iter),
     rep_pop = alps::replacement_layers(pop, iter),
     status = starting_status]() mutable
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
/// Sets the initial age of the population members.
///
template<Evaluator E>
template<Population P>
void alps_es<E>::init(P &pop)
{
  alps::set_age(pop);
}

///
/// Increments population's age and checks if it's time to add a new layer.
///
template<Evaluator E>
template<Population P>
void alps_es<E>::after_generation(
  unsigned generation,
  P &pop,
  const analyzer<individual_t, fitness_t> &az)
{
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
          layer->allowed(std::max(env.population.min_individuals,
                                  layer->size() / 2));
        else
          layer->allowed(env.population.individuals);

        ++layer;
      }
    }
  }

  // Code executed every `age_gap` interval.
  if (generation && generation % env.alps.age_gap == 0)
  {
    if (const auto n_layers(pop.layers());
        n_layers < env.alps.max_layers
        || az.age_dist(n_layers - 1).mean() > env.alps.max_age(n_layers))
      pop.add_layer();
    else
    {
      this->replace_.try_move_up_layer(pop.front(), pop.layer(1));
      pop.init(pop.front());
    }
  }
}

///
/// \param[out] env environemnt
/// \return         a strategy-specific environment
///
/// \remark
/// ALPS requires at least two layers.
///
template<Evaluator E>
environment alps_es<E>::shape(environment env)
{
  env.alps.max_layers = 8;
  return env;
}

template<Evaluator E>
std_es<E>::std_es(const problem &prob, const E &eva)
  : evolution_strategy<E>(prob, eva),
    select_(this->eva_, prob.env),
    recombine_(this->eva_, prob),
    replace_(this->eva_, prob.env)
{
}

///
/// \param[in] pop             a population. Operations are performed on
///                            sub-populations of `pop
/// \param[in] iter            iterator pointing to the main subpopulation
///                            we're going to work on
/// \param[in] starting_status the starting status (usually generated via
///                            `summary::starting_status()`)
/// \return                    a callable object encaspulating the ALPS
///                            algorithm
///
/// The standard evolutionary loop:
/// - select some individuals via tournament selection;
/// - create a new offspring individual;
/// - place the offspring into the original population (steady state)
///   replacing a bad individual.
///
/// This whole process repeats until the termination criteria is satisfied.
/// With any luck, it will produce an individual that solves the problem at
/// hand.
///
template<Evaluator E>
template<Population P>
auto std_es<E>::operations(
  P &pop, typename P::layer_iter iter,
  const evolution_status<individual_t, fitness_t> &starting_status) const
{
  Expects(pop.layers());
  Expects(iterator_of(iter, pop.range_of_layers()));

  return
    [this, &pop_layer = *iter, status = starting_status]() mutable
    {
      Ensures(!pop_layer.empty());

      const auto parents(this->select_(pop_layer));
      const auto offspring(this->recombine_(parents).front());
      this->replace_(pop_layer, offspring, status);
    };
}

#endif  // include guard
