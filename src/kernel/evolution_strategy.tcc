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

///
/// Constructs an evolution strategy.
///
/// \param[in] prob the optimisation problem
/// \param[in] eva  the evaluator used to assess individuals
///
/// Stores references to the evaluator and the optimisation problem. These
/// references are shared by all operators composing the strategy (selection,
/// recombination, replacement).
///
/// \remnark
/// No ownership is taken. No initialisation of the population is performed
/// here.
///
template<Evaluator E>
evolution_strategy<E>::evolution_strategy(const problem &prob, E &eva)
  : eva_(eva), prob_(prob)
{
}

///
/// Performs post-generation bookkeeping.
///
/// \tparam P population type
///
/// \param[in,out] pop the evolved population
/// \param[in]     sum summary statistics of the current generation
///
/// This method is invoked once per generation, after all evolutionary steps
/// have completed.
///
///  Responsibilities:
///  - increment the age of all individuals;
///  - detect stagnation based on:
///    - number of generations without improvement;
///    - fitness variance within layers;
///  - reset layers that are both stagnant and converged.
///
///  A layer is reset when (`and` of the points):
///  - `generation - last_improvement > max_stuck_gen`;
///  - the fitness variance of the layer is approximately zero.
///
/// \note Layer reset preserves the number of layers.
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

///
/// Constructs an ALPS evolution strategy.
///
/// \param[in] prob the optimisation problem
/// \param[in] eva  the evaluator used to assess individuals
///
/// Initialises ALPS-specific operators:
/// - age-layered selection;
/// - recombination;
/// - age-aware replacement.
///
template<Evaluator E>
alps_es<E>::alps_es(const problem &prob, E &eva)
  : evolution_strategy<E>(prob, eva),
    select_(this->eva_, prob.params),
    recombine_(this->eva_, prob),
    replace_(this->eva_, prob.params)
{
}

///
/// Assembles one ALPS evolutionary step.
///
/// \tparam P population type
///
/// \param[in] pop             a population. Operations are performed on
///                            sub-populations of `pop`
/// \param[in] iter            iterator to the active age layer
/// \param[in] starting_status evolutionary status snapshot
/// \return                    a callable performing one ALPS evolutionary step
///
/// Builds a callable object encapsulating one iteration of the ALPS
/// evolutionary process for a specific layer.
///
/// The returned callable performs:
/// - selection from the current layer and all younger layers;
/// - offspring generation via recombination;
/// - replacement restricted to the current layer.
///
/// \note Executing the callable mutates the population.
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
/// Initialises individual ages for ALPS.
///
/// \param[in,out] pop the population to initialise
///
/// Sets the initial age of all individuals in the population according to
/// ALPS rules.
///
template<Evaluator E>
template<Population P>
void alps_es<E>::init(P &pop)
{
  alps::set_age(pop);
}

///
/// Performs ALPS-specific post-generation updates.
///
/// \param[in,out] pop the population after evolution
/// \param[in]     sum summary statistics of the current generation
///
/// Responsibilities include:
/// - incrementing individual ages;
/// - merging equivalent layers;
/// - shrinking converged layers;
/// - restoring layer sizes when diversity returns;
/// - adding new layers at age gaps;
/// - moving individuals up when maximum layer count is reached.
///
/// \remark Structural changes occur only at generation boundaries.
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
/// Shapes parameters for ALPS evolution.
///
/// \param[out] params generic evolution parameters
/// \return            modified parameters suitable for ALPS
///
/// Adjusts generic parameters to values required by ALPS.
///
/// In particular enforces a minimum number of age layers.
///
/// \note At least two layers are required by ALPS.
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
/// Assembles a standard evolutionary step.
///
/// \tparam P population type
///
/// \param[in] pop             the population
/// \param[in] iter            iterator to the active layer
/// \param[in] starting_status evolutionary status snapshot
/// \return                    a callable performing one evolutionary step
///
/// Implements a steady-state evolutionary loop:
/// - tournament selection;
/// - recombination;
/// - replacement within the same layer.
///
/// \note No cross-layer interaction occurs.
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
/// Assembles a differential evolution step.
///
/// \tparam P population type
///
/// \param[in] pop             the population
/// \param[in] iter            iterator to the active layer
/// \param[in] starting_status evolutionary status snapshot
/// \return                    a callable performing one DE step
///
/// Implements differential evolution:
/// - selection produces both parents and a target individual;
/// - recombination generates a trial vector;
/// - replacement compares the trial against the target.
///
/// \note Replacement is performed on the selected target only.
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
