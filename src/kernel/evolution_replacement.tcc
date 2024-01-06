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

///
/// \param[in] layer a layer
/// \param[in] incoming an individual
///
/// We would like to add `incoming` in layer `layer`. The insertion will
/// take place if:
/// * `layer` is not full or...
/// * after a "kill tournament" selection, the worst individual found is
///   too old for `layer` while the incoming one is within the limits or...
/// * the worst individual has a lower fitness than the incoming one and
///   both are simultaneously within/outside the time frame of `layer`.
///
template<class T>
bool alps<T>::try_add_to_layer(unsigned layer, const T &incoming)
{
  using coord = typename population<T>::coord;

  auto &p(this->pop_);
  assert(layer < p.layers());

  if (p.individuals(layer) < p.allowed(layer))
  {
    p.add_to_layer(layer, incoming);  // layer not full... inserting incoming
    return true;
  }

  // Layer is full, can we replace an existing individual?
  const auto m_age(allowed_age(layer));

  // Well, let's see if the worst individual we can find with a tournament...
  coord c_worst{layer, random::sup(p.individuals(layer))};
  auto f_worst(this->eva_(p[c_worst]));

  auto rounds(p.get_problem().env.tournament_size);
  while (rounds--)
  {
    const coord c_x{layer, random::sup(p.individuals(layer))};
    const auto f_x(this->eva_(p[c_x]));

    if ((p[c_x].age() > p[c_worst].age() && p[c_x].age() > m_age) ||
        (p[c_worst].age() <= m_age && p[c_x].age() <= m_age &&
         f_x < f_worst))
    {
      c_worst = c_x;
      f_worst = f_x;
    }
  }

  // ... is worse than the incoming individual.
  if ((incoming.age() <= m_age && p[c_worst].age() > m_age) ||
      ((incoming.age() <= m_age || p[c_worst].age() > m_age) &&
       this->eva_(incoming) >= f_worst))
  {
    if (layer + 1 < p.layers())
      try_add_to_layer(layer + 1, p[c_worst]);
    p[c_worst] = incoming;

    return true;
  }

  return false;
}

///
/// \param[in] parent coordinates of the candidate parents.
///                   The list is sorted in descending fitness, so the
///                   last element is the coordinates of the worst individual
///                   of the tournament.
/// \param[in] offspring vector of the "children".
/// \param[in,out] s statistical summary.
///
/// Parameters from the environment:
/// * elitism is `true` => a new best individual is always inserted into the
///   population.
///
template<class T>
void alps<T>::run(
  const typename strategy<T>::parents_t &parent,
  const typename strategy<T>::offspring_t &offspring, summary<T> *s)
{
  const auto layer(std::max(parent[0].layer, parent[1].layer));
  const auto f_off(this->eva_(offspring[0]));
  const auto &pop(this->pop_);
  const auto elitism(pop.get_problem().env.elitism);

  Expects(elitism != trilean::unknown);

  bool ins;
#if defined(MUTUAL_IMPROVEMENT)
  // To protect the algorithm from the potential deleterious effect of intense
  // exploratory dynamics, we can use a constraint which mandate that an
  // individual must be better than both its parents before insertion into
  // the population.
  // See "Exploiting The Path of Least Resistance In Evolution" (Gearoid Murphy
  // and Conor Ryan).
  if (f_off > this->eva_(pop[parent[0]]) && f_off > this->eva_(pop[parent[1]]))
#endif
  {
    ins = try_add_to_layer(layer, offspring[0]);
  }

  if (f_off > s->best.score.fitness)
  {
    // Sometimes a new best individual is discovered in a lower layer but he is
    // too old for its layer and the random tournament may choose only "not
    // aged" individuals (i.e. individuals within the age limit of their layer).
    // When this happen the new best individual would be lost without the
    // command below.
    // There isn't an age limit for the last layer so try_add_to_layer will
    // always succeed.
    if (!ins && elitism == trilean::yes)
      try_add_to_layer(pop.layers() - 1, offspring[0]);

    s->last_imp           = s->gen;
    s->best.solution      = offspring[0];
    s->best.score.fitness = f_off;
  }
}
*/
#endif  // include guard
