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
/// \param[in] eva    current evaluator
/// \param[in] params access to replacement specific parameters
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const parameters &params)
  : eva_(eva), params_(params)
{
}

///
/// \param[in]  pop       a population
/// \param[in]  offspring collection of "children"
/// \param[out] status    current evolution status
///
/// Used parameters:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`;
///
template<Evaluator E>
template<Population P>
bool tournament<E>::operator()(
  P &pop, const evaluator_individual_t<E> &offspring,
  evolution_status<evaluator_individual_t<E>,
                   evaluator_fitness_t<E>> &status) const
{
  static_assert(std::is_same_v<evaluator_individual_t<E>,
                               typename P::value_type>);

  Expects(0 <= this->params_.evolution.elitism
          && this->params_.evolution.elitism <= 1);

  const auto rounds(this->params_.evolution.tournament_size);
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

  const auto off_fit(this->eva_(offspring));

  status.update_if_better(scored_individual(offspring, off_fit));

  const bool replace(off_fit > worst_fitness
                     || !random::boolean(this->params_.evolution.elitism));
  if (replace)
    pop[worst_coord] = offspring;

  return replace;
}

///
/// \param[in]  from starting layer
/// \param[out] to   destination layer
///
/// Try to move individuals from layer `from` to the upper layer (calling
/// `try_add_to_layer` for each individual).
///
template<Evaluator E>
template<SizedRandomAccessPopulation P>
void alps<E>::try_move_up_layer(const P &from, P &to)
{
  for (const auto &prg : from)
    try_add_to_layer(std::vector{std::ref(to)}, prg);
}

///
/// \param[in] pops     a collection of references to sub-populations. Can
///                     contain one or two elements. The first one
///                     (`pops.front()`) is the main/current layer; the second
///                     one, if available, is the upper level layer
/// \param[in] incoming an individual
///
/// We would like to add `incoming` in layer `pops.front()`. The insertion will
/// take place if:
/// - `pops.front()` is not full or...
/// - after a "kill tournament" selection, the worst individual found is
///   too old for its layer while the incoming one is within the limits or...
/// - the worst individual has a lower fitness than the incoming one and
///   both are simultaneously within/outside the time frame of the layer.
///
template<Evaluator E>
template<PopulationWithMutex P, Individual I>
bool alps<E>::try_add_to_layer(std::vector<std::reference_wrapper<P>> pops,
                               const I &incoming) const
{
  Expects(!pops.empty());
  Expects(incoming.is_valid());

  auto &pop(pops.front().get());

  I worst;
  assert(worst.empty());

  {
    std::lock_guard lock(pop.mutex());

    if (pop.size() < pop.allowed())
    {
      pop.push_back(incoming);
      return true;
    }

    // Layer is full, can we replace an existing individual?
    const auto m_age(pop.max_age());

    // Well, let's see if the worst individual we can find with a tournament...
    auto worst_coord(random::coord(pop));
    auto worst_fit(this->eva_(pop[worst_coord]));
    auto worst_age(pop[worst_coord].age());

    auto rounds(this->params_.evolution.tournament_size);
    assert(rounds);

    while (rounds--)
    {
      const auto trial_coord(random::coord(pop));
      const auto trial_fit(this->eva_(pop[trial_coord]));
      const auto trial_age(pop[trial_coord].age());

      if (trial_age > std::max(worst_age, m_age)
          || (std::max(worst_age, trial_age) <= m_age && trial_fit < worst_fit))
      {
        worst_coord = trial_coord;
        worst_fit = trial_fit;
        worst_age = trial_age;
      }
    }

    const bool replace_worst(
      (incoming.age() <= m_age && worst_age > m_age)
      || ((incoming.age() <= m_age || worst_age > m_age)
          && this->eva_(incoming) >= worst_fit));

    // ... is worse than the incoming individual.
    if (replace_worst)
    {
      worst = pop[worst_coord];
      pop[worst_coord] = incoming;
    }
  }

  if (pops.size() > 1 && !worst.empty())
    try_add_to_layer(std::vector{pops.back()}, worst);

  return !worst.empty();
}

///
/// \param[in]  pops      a collection of references to populations. Can
///                       contain one or two elements. The first one (`pop[0]`)
///                       is the main/current layer; the second one, if
///                       available, is the upper level layer
/// \param[in]  offspring the "incoming" individual
/// \param[out] status    current evolution status
///
/// Used parameters:
/// - `evolution.tournament_size`.
///
template<Evaluator E>
template<PopulationWithMutex P, Individual I>
void alps<E>::operator()(
  std::vector<std::reference_wrapper<P>> pops, const I &offspring,
  evolution_status<evaluator_individual_t<E>,
                   evaluator_fitness_t<E>> &status) const
{
  static_assert(std::is_same_v<I, typename P::value_type>);
  static_assert(std::is_same_v<I, evaluator_individual_t<E>>);

  Expects(0 < pops.size() && pops.size() <= 2);

  const bool ins(try_add_to_layer(pops, offspring));

  if (const auto f_off(this->eva_(offspring));
      status.update_if_better(scored_individual(offspring, f_off)))
  {
    if (!ins)
      try_add_to_layer(std::vector{pops.back()}, offspring);
  }
}

template<Evaluator E>
bool de<E>::operator()(evaluator_individual_t<E> &target,
                       const evaluator_individual_t<E> &offspring,
                       evolution_status<evaluator_individual_t<E>,
                                        evaluator_fitness_t<E>> &status) const
{
  const auto elitism(this->params_.evolution.elitism);
  Expects(0 <= elitism && elitism <= 1);

  const auto off_fit(this->eva_(offspring));

  status.update_if_better(scored_individual(offspring, off_fit));

  // The equality in `>=` helps the DE population to navigate the flat portion
  // of a fitness landscape and to reduce the possibility of population
  // becoming stagnated.
  if (const auto target_fit(this->eva_(target));
      off_fit >= target_fit || !random::boolean(elitism))
  {
    target = offspring;
    return true;
  }

  return false;
}

#endif  // include guard
