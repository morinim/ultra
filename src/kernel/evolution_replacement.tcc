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
/// \param[in]  eva   current evaluator
/// \param[in]  env   environment (for replacement specific parameters)
/// \param[out] stats statistics about the replacement strategy
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const environment &env,
                      summary<closure_arg_t<E>, closure_return_t<E>> &stats)
  : eva_(eva), env_(env), stats_(stats)
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
template<Population P, Individual I>
void tournament<E>::operator()(P &pop, const I &offspring) const
{
  static_assert(std::is_same_v<I, typename P::value_type>);
  static_assert(std::is_same_v<I, closure_arg_t<E>>);

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

  const auto off_fit(this->eva_(offspring));

  if (off_fit > this->stats_.best.score.fitness)
    this->stats_.update_best(offspring, off_fit);

  if (off_fit > worst_fitness || !random::boolean(this->env_.evolution.elitism))
    pop[worst_coord] = offspring;
}

///
/// \param[in]  from starting layer
/// \param[out] to   destination layer
///
/// Try to move individuals from layer `from` to the upper layer (calling
/// `try_add_to_layer` for each individual).
///
template<Evaluator E>
template<RandomAccessPopulation P>
void alps<E>::try_move_up_layer(const P &from, P &to)
{
  for (const auto &prg : from)
    try_add_to_layer(std::vector{std::ref(to)}, prg);
}

///
/// \param[in] pops     a collection of references to populations. Can contain
///                     one or two elements. The first one (`pops.front()`) is
///                     the main/current layer; the second one, if available,
///                     is the upper level layer
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
    auto worst_fit(this->eva_(worst));
    auto worst_age(pop[worst_coord].age());

    auto rounds(this->env_.evolution.tournament_size);
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

    bool replace_worst((incoming.age() <= m_age && worst_age > m_age)
                       || ((incoming.age() <= m_age || worst_age > m_age)
                           && this->eva_(incoming) >= worst_fit));
    if (replace_worst)
    {
      worst = pop[worst_coord];
      pop[worst_coord] = incoming;
    }
  }

  // ... is worse than the incoming individual.
  if (!worst.empty() && pops.size() > 1)
    try_add_to_layer(std::vector{pops.back()}, worst);

  return !worst.empty();
}

///
/// \param[in] pops      a collection of references to populations. Can contain
///                      one or two elements. The first one (`pop[0]`) is the
///                      main/current layer; the second one, if available, is
///                      the upper level layer
/// \param[in] offspring a range containing the offspring
///
/// Parameters from the environment:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`.
///
template<Evaluator E>
template<PopulationWithMutex P, Individual I>
void alps<E>::operator()(std::vector<std::reference_wrapper<P>> pops,
                         const I &offspring) const
{
  static_assert(std::is_same_v<I, typename P::value_type>);
  static_assert(std::is_same_v<I, closure_arg_t<E>>);

  Expects(0 < pops.size() && pops.size() <= 2);
  Expects(0<= this->env_.evolution.elitism && this->env_.evolution.elitism<= 1);

  const bool ins(try_add_to_layer(pops, offspring));

  if (const auto f_off(this->eva_(offspring));
      f_off > this->stats_.best.score.fitness)
  {
    this->stats_.update_best(offspring, f_off);

    if (!ins)
      try_add_to_layer(std::vector{pops.back()}, offspring);
  }
}

template<Evaluator E>
template<PopulationWithMutex P, Individual I>
void alps<E>::operator()(std::initializer_list<std::reference_wrapper<P>> pops,
                         const I &offspring) const
{
  operator()(std::vector(pops), offspring);
}

#endif  // include guard
