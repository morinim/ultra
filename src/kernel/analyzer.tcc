/**
 *  \file
 *  \remark This file is part of VITA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_ANALYZER_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_ANALYZER_TCC)
#define      ULTRA_ANALYZER_TCC

///
/// Resets gathered statics.
///
template<Individual I, Fitness F>
void analyzer<I, F>::clear()
{
  *this = {};
}

///
/// \return statistics about the age distribution of the individuals
///
template<Individual I, Fitness F>
const distribution<double> &analyzer<I, F>::age_dist() const
{
  return age_;
}

///
/// \param[in] g a population / subpopulation
/// \return      statistics about the age distribution of individuals in group
///              `g`
///
template<Individual I, Fitness F>
template<Population P>
const distribution<double> &analyzer<I, F>::age_dist(const P &g) const
{
  const auto gi(group_stat_.find(g.uid()));
  assert(gi != group_stat_.end());

  return gi->second.age;
}

///
/// \return statistics about the fitness distribution of the individuals
///
template<Individual I, Fitness F>
const distribution<F> &analyzer<I, F>::fit_dist() const
{
  return fit_;
}

///
/// \param[in] g a population / subpopulation
/// \return      statistics about the fitness distribution of individuals in
///              group `g`
///
template<Individual I, Fitness F>
template<Population P>
const distribution<F> &analyzer<I, F>::fit_dist(const P &g) const
{
  const auto gi(group_stat_.find(g.uid()));
  assert(gi != group_stat_.end());

  return gi->second.fitness;
}

///
/// \return statistic about the length distribution of the individuals
///
template<Individual I, Fitness F>
const distribution<double> &analyzer<I, F>::length_dist() const
{
  return length_;
}

///
/// \param[in] g a population / subpopulation
/// \return      statistics about the length distribution of individuals in
///              group `g`
///
template<Individual I, Fitness F>
template<Population P>
const distribution<double> &analyzer<I, F>::length_dist(const P &g) const
{
  const auto gi(group_stat_.find(g.uid()));
  assert(gi != group_stat_.end());

  return gi->second.length;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I, Fitness F>
bool analyzer<I, F>::is_valid() const
{
  return true;
}

///
/// Adds a new individual to the pool used to calculate statistics.
///
/// \param[in] ind new individual
/// \param[in] f   fitness of the new individual
/// \param[in] g   a group of the population
///
/// The optional `g` parameter can be used to group information (e.g. for the
/// ALPS algorithm it's used for layer specific statistics).
///
template<Individual I, Fitness F>
void analyzer<I, F>::add(const I &ind, const F &f, unsigned g)
{
  age_.add(ind.age());
  group_stat_[g].age.add(ind.age());

  const auto len(active_slots(ind));
  length_.add(len);
  group_stat_[g].length.add(len);

  if (isfinite(f))
  {
    fit_.add(f);
    group_stat_[g].fitness.add(f);
  }
}

///
/// \param[in] pop a population
/// \param[in] eva an evaluator
/// \return        compiled analyzer for the given population
///
template<Population P, Evaluator E>
[[nodiscard]] auto analyze(const P &pop, const E &eva)
{
  analyzer<evaluator_individual_t<E>, evaluator_fitness_t<E>> az;

  for (auto it(pop.begin()), end(pop.end()); it != end; ++it)
    az.add(*it, eva(*it), it.uid());

  return az;
}

#endif  // include guard
