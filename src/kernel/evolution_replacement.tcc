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
/// \param[in]  offspring the child to be considered for replacement
/// \param[out] status    current evolution status
///
/// Used parameters:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`;
///
template<Evaluator E>
template<Population P>
bool tournament<E>::operator()(P &pop, const individual_t &offspring,
                               status_t &status) const
{
  static_assert(std::is_same_v<individual_t, typename P::value_type>);

  Expects(!this->params_.needs_init());

  const auto rounds(this->params_.evolution.tournament_size);
  assert(0 < rounds);
  assert(rounds <= parameters::evolution_parameters::max_tournament_size);

  auto worst_coord(random::coord(pop));
  auto worst_fitness(this->eva_(pop[worst_coord]));

  for (std::size_t i(1); i < rounds; ++i)
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

  const auto elitism(this->params_.evolution.elitism);
  assert(in_0_1(elitism));

  const bool replace(off_fit > worst_fitness || !random::boolean(elitism));
  if (replace)
    pop[worst_coord] = offspring;

  return replace;
}

///
/// \param[in]  from starting layer
/// \param[out] to   destination layer
///
/// Try to promote individuals from layer `from` to `to` (with ALPS this is
/// always the upper layer).
///
template<Evaluator E>
template<PopulationWithMutex P>
void alps<E>::try_promote_individuals(const P &from, P &to) const
{
  for (const auto &prg : from)
    try_add_to_layer(to, prg);
}

///
/// Attempts to insert `incoming` into the primary layer.
///
/// \param[in] pops     a collection of references to sub-populations. It may
///                     contain one or two layers. `pops.primary()` is the
///                     current layer; `pops.secondary()`, when present, is the
///                     next (older) layer
/// \param[in] incoming the individual that we attempt to insert in the current
///                     layer
/// \return             `true` if `incoming` has been accepted into the primary
///                     layer (either appended or replacing a resident);
///                     `false` otherwise
///
/// The insertion succeeds if one of the following conditions holds:
/// - the layer still has free capacity;
/// - a "kill tournament" selects a resident that is too old for the layer
///   while `incoming` is still within the allowed age;
/// - both individuals are either valid or invalid for the layer (with respect
///   to age) and `incoming` has a fitness not worse than the selected one.
///
/// If the layer is full and replacement occurs, the removed individual may be
/// forwarded to the secondary layer (when present).
///
/// \note
/// The return value only reflects whether `incoming` entered the primary
/// layer. If a resident individual is displaced, an attempt is made to insert
/// it into the secondary layer, but the success or failure of that promotion
/// doesn't affect the return value.
///
template<Evaluator E>
template<PopulationWithMutex P>
bool alps<E>::try_add_to_layer(alps_layer_pair<P> pops,
                               const individual_t &incoming) const
{
  static_assert(std::is_same_v<individual_t, typename P::value_type>);
  Expects(incoming.is_valid());

  auto &pop(pops.primary());

  if (!pop.allowed())
    return false;

  // ---- Check capacity and append if there is room ----
  if (std::lock_guard lock(pop.mutex()); pop.size() < pop.allowed())
  {
    pop.push_back(incoming);
    return true;
  }

  // Layer is full, can we replace an existing individual?
  individual_t displaced;
  assert(displaced.empty());

  // ---- Snapshot candidates then evaluate (slow, no lock) ----
  struct snapshot_t
  {
    typename P::coord coord {};
    individual_t ind {};
  };

  const std::size_t ts(this->params_.evolution.tournament_size);
  Expects(ts > 0);
  Expects(ts <= parameters::evolution_parameters::max_tournament_size);

  std::vector<snapshot_t> candidates;
  candidates.reserve(ts);

  const auto max_age([&]
  {
    std::shared_lock<std::shared_mutex> lock(pop.mutex());

    // Sample `ts` candidates (with replacement). Copy individuals out.
    for (std::size_t i(0); i < ts; ++i)
    {
      const auto c(random::coord(pop));
      const auto &cur(pop[c]);

      candidates.push_back({c, cur});
    }

    return pop.max_age();
  }());

  // Evaluate copied candidates outside any population lock.
  std::size_t worst_idx(0);
  auto worst_fit(this->eva_(candidates[0].ind));
  auto worst_age(candidates[0].ind.age());

  for (std::size_t i(1); i < ts; ++i)
  {
    const auto trial_fit(this->eva_(candidates[i].ind));
    const auto trial_age(candidates[i].ind.age());

    if (trial_age > std::max(worst_age, max_age)
        || (std::max(worst_age, trial_age) <= max_age && trial_fit < worst_fit))
    {
      worst_idx = i;
      worst_fit = trial_fit;
      worst_age = trial_age;
    }
  }

  bool replace_worst(false);

  if (const auto incoming_age(incoming.age());
      incoming_age <= max_age && worst_age > max_age)  // age rule
  {
    // Incoming fits the layer, worst doesn't.
    replace_worst = true;
  }
  else if (incoming_age > max_age && worst_age <= max_age)  // age rule
  {
    // Incoming is too old but worst is still valid.
    replace_worst = false;
  }
  else
  {
    const auto incoming_fit(this->eva_(incoming));
    replace_worst = incoming_fit >= worst_fit;
  }

  // ---- Commit (exclusive) ----
  if (replace_worst)
  {
    std::lock_guard lock(pop.mutex());

    const auto coord(candidates[worst_idx].coord);

    if (const bool changed(pop[coord] != candidates[worst_idx].ind); changed)
    {
      // In ALPS, every non-final layer is owned by a single thread, so a
      // snapshot/commit mismatch there should never happen and indicates a
      // logic error. The final layer is different: multiple lower layers may
      // concurrently promote individuals into it, so the sampled resident may
      // legitimately change before commit.
      if (pops.has_secondary())
      {
        ultraERROR << "ALPS snapshot/commit mismatch in non-final layer"
                   << pop.uid();
      }

      return false;
    }

    displaced = pop[coord];
    pop[coord] = incoming;
  }

  if (pops.has_secondary() && !displaced.empty())
    try_add_to_layer(pops.secondary(), displaced);

  return !displaced.empty();
}

template<Evaluator E>
template<PopulationWithMutex P>
bool alps<E>::try_add_to_layer(P &layer, const individual_t &incoming) const
{
  return try_add_to_layer(alps_layer_pair(layer), incoming);
}

///
/// \param[in]  pops      a collection of references to populations. Can
///                       contain one or two elements. The first one (`pop[0]`)
///                       is the main/current layer; the second one, if
///                       available, is the next layer
/// \param[in]  offspring the "incoming" individual
/// \param[out] status    current evolution status
///
/// Used parameters:
/// - `evolution.elitism`;
/// - `evolution.tournament_size`.
///
template<Evaluator E>
template<PopulationWithMutex P>
void alps<E>::operator()(alps_layer_pair<P> pops,
                         const individual_t &offspring, status_t &status) const
{
  static_assert(std::is_same_v<individual_t, typename P::value_type>);
  Expects(!this->params_.needs_init());

  const bool ins(try_add_to_layer(pops, offspring));

  const auto f_off(this->eva_(offspring));
  const bool new_best(
    status.update_if_better(scored_individual(offspring, f_off)));

  if (ins || !new_best || !pops.has_secondary())
    return;

  // If a rejected offspring is a new global best, try to preserve it in the
  // last layer. The number of retries depends on the elitism parameter
  // (`elitism == 0` disables this rescue attempt).
  const auto elitism(this->params_.evolution.elitism);
  assert(in_0_1(elitism));

  auto retries(static_cast<unsigned>(std::round(elitism * 3.0)));
  while (retries--)
    if (try_add_to_layer(pops.secondary(), offspring))
      break;
}

template<Evaluator E>
bool de<E>::operator()(individual_t &target, const individual_t &offspring,
                       status_t &status) const
{
  Expects(!this->params_.needs_init());

  const auto off_fit(this->eva_(offspring));
  status.update_if_better(scored_individual(offspring, off_fit));

  // The equality in `>=` helps the DE population to navigate the flat portion
  // of a fitness landscape and to reduce the possibility of population
  // becoming stagnant.
  const auto elitism(this->params_.evolution.elitism);
  assert(in_0_1(elitism));

  if (const auto target_fit(this->eva_(target));
      off_fit >= target_fit || !random::boolean(elitism))
  {
    target = offspring;
    return true;
  }

  return false;
}

#endif  // include guard
