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

#if !defined(ULTRA_EVOLUTION_SELECTION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_SELECTION_TCC)
#define      ULTRA_EVOLUTION_SELECTION_TCC

///
/// \param[in] eva    current evaluator
/// \param[in] params access to selection specific parameters
///
/// \warning
/// The lifetime of `eva` must exceed lifetime of `this` class.
///
template<Evaluator E>
strategy<E>::strategy(E &eva, const parameters &params)
  : eva_(eva), params_(params)
{
}

///
/// \param[in] pop a population
/// \return        a collection of scored individuals ordered in descending
///                fitness
///
/// \pre Parameters must have been initialised (no auto-tune values)
///
/// Tournament selection works by selecting a number of individuals from the
/// population at random (a tournament) and then choosing only the best of
/// those individuals.
/// Recall that better individuals have higher fitness.
///
/// Used parameters: `evolution.mate_zone`, `evolution.tournament_size`.
///
/// \remark
/// Different compilers may optimize the code producing slightly different
/// sortings (due to floating point approximations). This is a known *issue*.
/// Anyway we keep using the `<` operator because:
/// - it's faster than the `std::fabs(delta)` approach;
/// - the additional *noise* is marginal (for the GA/GP standard);
/// - for debugging purposes *compiler-stability* is enough (and we have faith
///   in the test suite).
///
/// \warning
/// This function assumes exclusive access to the population. If other threads
/// can change the individuals, race conditions may happen.
///
template<Evaluator E>
template<SizedRandomAccessPopulation P>
std::vector<typename tournament<E>::scored_t>
tournament<E>::operator()(const P &pop) const
{
  Expects(!this->params_.needs_init());

  const auto rounds(this->params_.evolution.tournament_size);
  assert(0 < rounds);
  assert(rounds <= parameters::evolution_parameters::max_tournament_size);

  std::vector<std::pair<typename P::coord, evaluator_fitness_t<E>>> ret;
  ret.reserve(rounds);

  // This is the inner loop of an insertion sort algorithm. It's simple, fast
  // (if `rounds` is small) and doesn't perform too much comparisons.
  // DO NOT USE `std::sort` it's way slower.
  const auto mate_zone(this->params_.evolution.mate_zone);
  const auto target(random::coord(pop));

  for (std::size_t i(0); i < rounds; ++i)
  {
    const auto new_coord(random::coord(pop, target, mate_zone));
    const auto new_fitness(this->eva_(pop[new_coord]));

    ret.emplace_back(new_coord, new_fitness);

    auto k(i);
    while (k && ret[k - 1].second < ret[k].second)
    {
      std::swap(ret[k - 1], ret[k]);
      --k;
    }
  }

  assert(std::ranges::is_sorted(ret,
                                [](const auto &i1, const auto &i2)
                                {
                                  return i1.second > i2.second;
                                }));

  std::vector<scored_t> rs;
  rs.reserve(ret.size());
  std::ranges::transform(ret, std::back_inserter(rs),
                         [&pop](const auto &c)
                         {
                           return scored_t{pop[c.first], c.second};
                         });

  Ensures(rs.size() == rounds);
  return rs;
}

///
/// Selects two parents using ALPS tournament selection.
///
/// \param[in] pops pair of eligible layers. `pops.primary()` is the
///                 current/main layer; `pops.secondary()` (if present) is the
///                 immediately younger layer
/// \return         two selected scored individuals (best first)
///
/// \pre Parameters must have been initialised (no auto-tune values)
///
/// Used parameters:
/// - `evolution.tournament_size` to control number of selected individuals.
/// - `alps.p_main_layer` (probability of sampling each additional tournament
///   candidate from the primary layer when a secondary layer is present).
///
/// The first two candidates are always sampled from the primary layer. The
/// remaining `evolution.tournament_size - 1` candidates use
/// `alps.p_main_layer`.
///
/// \note
/// This function can be used in a multithreaded environment. Individuals are
/// sampled through `random::individual`, which acquires a shared lock on the
/// population mutex and returns a copy of the selected individual. The
/// selection logic therefore operates on local copies and does not access
/// mutable population state after sampling.
///
/// Correctness assumes that:
/// - population mutations acquire the same mutex with exclusive access;
/// - any additional population queries used here (e.g. `max_age()`) are
///   either immutable or internally synchronised.
///
template<Evaluator E>
template<PopulationWithMutex P>
std::array<typename alps<E>::scored_t, 2>
alps<E>::operator()(alps_layer_pair<const P> pops) const
{
  Expects(!this->params_.needs_init());

  struct candidate
  {
    evaluator_individual_t<E> ind;

    // Extends the basic fitness with the age and takes advantage of the
    // lexicographic comparison capabilities of `std::pair`.
    std::pair<bool, evaluator_fitness_t<E>> rank;
  };

  const auto sample([this](const auto &layer)
  {
    auto ind(random::individual(layer));
    const auto rank(std::pair(ind.age() <= layer.max_age(),
                              this->eva_(ind)));

    return candidate{std::move(ind), rank};
  });

  auto best(sample(pops.primary()));
  auto second(sample(pops.primary()));

  if (best.rank < second.rank)
    std::swap(best, second);

  const auto ts(this->params_.evolution.tournament_size);
  assert(0<ts && ts<=parameters::evolution_parameters::max_tournament_size);

  const auto p_main_layer(this->params_.alps.p_main_layer);
  assert(in_0_1(p_main_layer));

  for (auto rounds(ts - 1); rounds; --rounds)
  {
    auto incoming(sample(pops.random(p_main_layer)));

    if (second.rank < incoming.rank)
    {
      second = std::move(incoming);

      if (best.rank < second.rank)
        std::swap(best, second);
    }

    assert(best.rank >= second.rank);
    assert(best.rank.first || !second.rank.first);
  }

  return  {scored_t{std::move(best.ind), best.rank.second},
           scored_t{std::move(second.ind), second.rank.second}};
}

///
/// \param[in] pop a population
/// \return        references of four individuals suited for DE recombination.
///                First and second elements are parents, the remaining ones
///                are used for scaling.
///
/// \warning
/// This function assumes exclusive access to the population. If other threads
/// can change the individuals, race conditions may happen.
///
template<SizedRandomAccessPopulation P>
de::selected_refs<typename P::value_type>
de::operator()(P &pop) const
{
  const auto target(random::coord(pop));
  const auto base(random::coord(pop));
  const auto a(random::coord(pop));
  const auto b(random::coord(pop));

  // We don't force constraints on indices.

  return {pop[target], pop[base], pop[a], pop[b]};
}

#endif  // include guard
