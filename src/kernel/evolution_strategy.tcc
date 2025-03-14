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
evolution_strategy<E>::evolution_strategy(const problem &prob, E &eva)
  : eva_(eva), prob_(prob)
{
}

///
/// We use an accelerated stop condition when:
/// - after `max_stuck_gen` generations the situation doesn't change;
/// - all the individuals have the same fitness.
///
template<Evaluator E>
template<Population P>
void evolution_strategy<E>::after_generation(
  P &pop, const summary<individual_t, fitness_t> &sum)
{
  Expects(&pop.problem() == &this->get_problem());
  const auto &params(pop.problem().params);
  Expects(params.evolution.max_stuck_gen);

  pop.inc_age();

  for (const auto layers(pop.range_of_layers()); auto &layer : layers)
  {
    // Pay attention to `params.max_stuck_gen`: it's often a large number and
    // can cause overflow. E.g.
    // `sum.generation > sum.last_improvement + params.max_stuck_gen`
    if (sum.generation - sum.last_improvement() > params.evolution.max_stuck_gen
        && issmall(sum.az.fit_dist(layer).variance()))
    {
      layer.reset(pop.problem());
      ultraINFO << "Resetting layer " << layer.uid();
    }
  }
}

template<Evaluator E>
alps_es<E>::alps_es(const problem &prob, E &eva)
  : evolution_strategy<E>(prob, eva),
    select_(this->eva_, prob.params),
    recombine_(this->eva_, prob),
    replace_(this->eva_, prob.params)
{
}

///
/// \param[in] pop             a population. Operations are performed on
///                            sub-populations of `pop`
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
      const auto offspring(this->recombine_(parents));
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
void alps_es<E>::after_generation(P &pop,
                                  const summary<individual_t, fitness_t> &sum)
{
  Expects(&pop.problem() == &this->get_problem());
  const auto &params(pop.problem().params);

  pop.inc_age();

  const auto layers(pop.range_of_layers());

  if (pop.layers() > 1)
  {
    for (auto layer(std::next(layers.begin())); layer != layers.end();)
    {
      if (almost_equal(sum.az.fit_dist(*std::prev(layer)).mean(),
                       sum.az.fit_dist(*layer).mean()))
      {
        ultraDEBUG << "ALPS: erasing layer UID=" << layer->uid();
        layer = pop.erase(layer);
      }
      else
      {
        const auto sd(sum.az.fit_dist(*layer).standard_deviation());
        const bool converged(issmall(sd));

        if (converged)
        {
          const auto prev_allowed(layer->allowed());
          const auto new_allowed(std::max(params.population.min_individuals,
                                          layer->size() / 2));

          if (new_allowed < prev_allowed)
          {
            ultraDEBUG << "ALPS: decreasing allowed individuals of layer UID="
                       << layer->uid() << " to " << new_allowed;

            layer->allowed(new_allowed);
          }
        }
        else
        {
          if (layer->allowed() < params.population.individuals)
          {
            ultraDEBUG << "ALPS: restoring allowed individuals of layer UID="
                       << layer->uid() << " to "
                       << params.population.individuals;

            layer->allowed(params.population.individuals);
          }
        }

        ++layer;
      }
    }
  }

  // Code executed every `age_gap` interval.
  if (sum.generation && sum.generation % params.alps.age_gap == 0)
  {
    if (const auto n_layers(pop.layers());
        n_layers < params.alps.max_layers
        || sum.az.age_dist(pop.back()).mean() > params.alps.max_age(n_layers))
    {
      ultraDEBUG << "ALPS: adding layer";
      pop.add_layer();
    }
    else
    {
      ultraDEBUG << "ALPS: try moving up first layer";
      this->replace_.try_move_up_layer(pop.front(), pop.layer(1));
      pop.init(pop.front());
    }
  }
}

///
/// \param[out] params generic parameters
/// \return            strategy specific parameters
///
/// \remark
/// ALPS requires at least two layers.
///
template<Evaluator E>
parameters alps_es<E>::shape(parameters params)
{
  params.alps.max_layers = 8;
  return params;
}

template<Evaluator E>
std_es<E>::std_es(const problem &prob, E &eva)
  : evolution_strategy<E>(prob, eva),
    select_(this->eva_, prob.params),
    recombine_(this->eva_, prob),
    replace_(this->eva_, prob.params)
{
}

///
/// \param[in] pop             a population. Operations are performed on
///                            sub-groups of `pop`
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
  [[maybe_unused]] P &pop, typename P::layer_iter iter,
  const evolution_status<individual_t, fitness_t> &starting_status) const
{
  Expects(pop.layers());
  Expects(iterator_of(iter, pop.range_of_layers()));

  return
    [this, &pop_layer = *iter, status = starting_status]() mutable
    {
      Ensures(!pop_layer.empty());

      const auto parents(this->select_(pop_layer));
      const auto offspring(this->recombine_(parents));
      this->replace_(pop_layer, offspring, status);
    };
}

template<Evaluator E>
de_es<E>::de_es(const problem &prob, E &eva)
  : evolution_strategy<E>(prob, eva),
    recombine_(prob),
    replace_(this->eva_, prob.params)
{
}

///
/// \param[in] pop             a population. Operations are performed on
///                            sub-populations of `pop`
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
auto de_es<E>::operations(
  [[maybe_unused]]  P &pop, typename P::layer_iter iter,
  const evolution_status<individual_t, fitness_t> &starting_status) const
{
  Expects(pop.layers());
  Expects(iterator_of(iter, pop.range_of_layers()));

  return
    [this, &pop_layer = *iter, status = starting_status]() mutable
    {
      Ensures(!pop_layer.empty());

      const auto selected(this->select_(pop_layer));
      const auto offspring(this->recombine_(selected));
      this->replace_(selected.target, offspring, status);
    };
}

#endif  // include guard
