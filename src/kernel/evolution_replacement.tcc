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
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_REPLACEMENT_TCC)
#define      ULTRA_EVOLUTION_REPLACEMENT_TCC

///
/// \param[in] eva current evaluator
/// \param[in] env environment (for replacement specific parameters)
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const environment &env,
                      summary<closure_arg_t<E>, closure_return_t<E>> *stats)
  : eva_(eva), env_(env), stats_(*stats)
{
}

///
/// \param[in] pop       a population
/// \param[in] offspring collection of "children".
///
/// Parameters from the environment:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`;
///
template<Evaluator E>
template<Population P, SizedRangeOfIndividuals R>
void tournament<E>::operator()(P &pop, const R &offspring) const
{
  static_assert(std::is_same_v<typename P::value_type,
                               std::ranges::range_value_t<R>>);
  static_assert(std::is_same_v<typename P::value_type, closure_arg_t<E>>);

  Expects(offspring.size() == 1);
  Expects(0<= this->env_.evolution.elitism && this->env_.evolution.elitism<= 1);

  const auto rounds(this->env_.evolution.tournament_size);
  assert(rounds);

  auto worst_coord(random::coord(pop));
  auto worst_fitness(this->eva_(pop[worst_coord]));

  for (unsigned i(1); i < rounds; ++i)
  {
    const auto new_coord(random::coord(pop));
    const auto new_fitness(this->eva_(pop[new_coord]));

    if (new_fitness < worst_fitness)
    {
      worst_fitness = new_fitness;
      worst_coord = new_coord;
    }
  }

  const auto off_fit(this->eva_(offspring[0]));
  if (off_fit > this->stats_.best.score.fitness)
    this->stats_.update_best(offspring[0], off_fit);

  if (!random::boolean(this->env_.evolution.elitism) || off_fit > worst_fitness)
    pop[worst_coord] = offspring[0];
}

/*
///
/// \param[in] l a layer
/// \return    the maximum allowed age for an individual in layer `l`
///
/// This is just a convenience method to save some keystroke.
///
template<class T>
unsigned alps<T>::allowed_age(unsigned l) const
{
  const auto &pop(this->pop_);

  return pop.get_problem().env.alps.allowed_age(l, pop.layers());
}

///
/// \param[in] l a layer
///
/// Try to move individuals in layer `l` in the upper layer (calling
/// try_add_to_layer for each individual).
///
template<class T>
void alps<T>::try_move_up_layer(unsigned l)
{
  auto &pop(this->pop_);

  if (l + 1 < pop.layers())
  {
    const auto n(pop.individuals(l));

    for (auto i(decltype(n){0}); i < n; ++i)
      try_add_to_layer(l + 1, pop[{l, i}]);
  }
}
*/

///
/// \param[in] pops     a collection of references to populations. Can contain
///                     one or two elements. The first one (`pop[0]`) is the
///                     main/current layer; the second one, if available, is
///                     the upper level layer
/// \param[in] incoming an individual
///
/// We would like to add `incoming` in layer `pops[0]`. The insertion will
/// take place if:
/// - `pop[0]` is not full or...
/// - after a "kill tournament" selection, the worst individual found is
///   too old for `layer` while the incoming one is within the limits or...
/// - the worst individual has a lower fitness than the incoming one and
///   both are simultaneously within/outside the time frame of the layer.
///
template<Evaluator E>
template<PopulationWithMutex P, Individual I>
bool alps<E>::try_add_to_layer(std::vector<std::reference_wrapper<P>> pops,
                               const I &incoming) const
{
  const auto pop(pops.front().get());

  I worst;
  assert(worst.empty());

  {
    std::lock_guard lock(pop.mutex());

    if (pop.size < pop.allowed())
    {
      pop.push_back(incoming);
      return true;
    }

    // Layer is full, can we replace an existing individual?
    const auto m_age(pop.max_age());

    // Well, let's see if the worst individual we can find with a tournament...
    auto worst_coord(random::coord(pop));
    auto worst_fit(this->eva_(worst));

    auto rounds(this->env_.evolution.tournament_size);
    assert(rounds);

    while (rounds--)
    {
      const auto trial_coord(random::coord(pop));
      const auto trial_fit(this->eva_(pop[trial_coord]));

      if (pop[trial_coord].age() > std::max(pop[worst_coord].age(), m_age)
          || (std::max(pop[worst_coord].age(), pop[trial_coord].age()) <= m_age
              && trial_fit < worst_fit))
      {
        worst_coord = trial_coord;
        worst_fit = trial_fit;
      }
    }

    bool worst_replaced((incoming.age() <= m_age && worst.age() > m_age)
                        || ((incoming.age() <= m_age || worst.age() > m_age)
                            && this->eva_(incoming) >= worst_fit));
    if (worst_replaced)
    {
      worst = pop[worst_coord];
      pop[worst_coord] = incoming;
    }
  }

  // ... is worse than the incoming individual.
  if (!worst.empty() && pops.size() > 1)
    try_add_to_layer(std::vector{pops[1]}, worst);

  return !worst.empty();
}

///
/// \param[in] pops      a collection of references to populations. Can contain
///                      one or two elements. The first one (`pop[0]`) is the
///                      main/current layer; the second one, if available, is
///                      the upper level layer
/// \param[in] offspring a range of the "children"
///
/// Parameters from the environment:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`.
///
template<Evaluator E>
template<PopulationWithMutex P, SizedRangeOfIndividuals R>
void alps<E>::operator()(std::vector<std::reference_wrapper<P>> pops,
                         const R &offspring) const
{
  static_assert(std::is_same_v<typename P::value_type,
                               std::ranges::range_value_t<R>>);
  static_assert(std::is_same_v<typename P::value_type, closure_arg_t<E>>);

  Expects(pops.size() == 2);
  Expects(offspring.size() == 1);
  Expects(0<= this->env_.evolution.elitism && this->env_.evolution.elitism<= 1);

  try_add_to_layer(pops, offspring[0]);

  if (const auto f_off(this->eva_(offspring[0]));
      f_off > this->stats_.best.score.fitness)
    this->stats_.update_best(offspring[0], f_off);
}

#endif  // include guard
