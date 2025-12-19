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

#if !defined(ULTRA_ANALYZER_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_ANALYZER_TCC)
#define      ULTRA_ANALYZER_TCC

namespace internal
{

inline std::map<int, unsigned> merge_ct(const auto &ct1, const auto &ct2)
{
  std::map<int, unsigned> ret(ct1);

  for (const auto &[ct, n] : ct2)
    ret[ct] += n;

  return ret;
}

}  // namespace internal

template<class C> concept has_active_crossover_type = requires(const C &c)
{
  { c.active_crossover_type() } -> std::convertible_to<int>;
};

template<Individual I, Fitness F>
group_stat<I, F>::group_stat(population_uid id) : uid(id)
{
}

///
/// Adds a new individual to the pool used to calculate statistics.
///
/// \param[in] ind new individual
/// \param[in] f   fitness of the new individual
///
template<Individual I, Fitness F>
void group_stat<I, F>::add(const I &ind, const F &f)
{
  age.add(ind.age());

  using de::active_slots;
  using gp::active_slots;

  const std::size_t len(active_slots(ind));

  length.add(len);

  using std::isfinite;
  if (isfinite(f))
    fitness.add(f);

  if constexpr (has_active_crossover_type<I>)
    ++crossover_type[ind.active_crossover_type()];
}

template<Individual I, Fitness F>
void group_stat<I, F>::merge(group_stat gs)
{
  group_stat<I, F> ret;

  age.merge(gs.age);
  fitness.merge(gs.fitness);
  length.merge(gs.length);

  crossover_type = internal::merge_ct(crossover_type, gs.crossover_type);
}

///
/// Calculates statistics about a layered population.
///
/// \param[in] pop the layered population
///
template<Individual I, Fitness F>
template<LayeredPopulation P, Evaluator E> analyzer<I, F>::analyzer(
  const P &pop, const E &eva)
{
  const auto gather_subgroup_stats([&eva](auto layer_iter)
  {
    group_stat<I, F> ret(layer_iter->uid());

    for (const auto &ind : *layer_iter)
      ret.add(ind, eva(ind));

    return ret;
  });

  const auto range(pop.range_of_layers());
  std::vector<std::future<group_stat<I, F>>> tasks;
  for (auto l(range.begin()); l != range.end(); ++l)
    tasks.push_back(std::async(std::launch::async, gather_subgroup_stats, l));

  for (auto &task : tasks)
    group_stat_.push_back(task.get());
}

///
/// Resets gathered statics.
///
template<Individual I, Fitness F>
void analyzer<I, F>::clear()
{
  *this = {};
}

template<Individual I, Fitness F>
const group_stat<I, F> *analyzer<I, F>::group(population_uid uid) const
{
  const auto iter(std::ranges::find_if(group_stat_,
                                       [uid](const auto &grp)
                                       {
                                         return grp.uid == uid;
                                       }));

  return iter == group_stat_.end() ? nullptr : std::addressof(*iter);
}

template<Individual I, Fitness F>
group_stat<I, F> *analyzer<I, F>::group(population_uid uid)
{
  // My goodness... however, nobody ever got fired for following Scott Meyers.
  return const_cast<group_stat<I, F> *>(std::as_const(*this).group(uid));
}

///
/// \return aggregate `group_stat` considering every subgroup of the population
///
template<Individual I, Fitness F>
group_stat<I, F> analyzer<I, F>::overall_group_stat() const
{
  group_stat<I, F> ret;

  for (const auto &gs : group_stat_)
    ret.merge(gs);

  return ret;
}

///
/// \return statistics about the crossover operators.
///         The `.empty()` method of the returned value can be `true`
///
template<Individual I, Fitness F>
auto analyzer<I, F>::crossover_types() const
{
  decltype(group_stat<I, F>::crossover_type) ret;
  for (const auto &gs : group_stat_)
    ret = internal::merge_ct(ret, gs.crossover_type);

  return ret;
}

///
/// \param[in] g the UID of a population / subpopulation
/// \return      statistics about the crossover operators used in group `g`
///
template<Individual I, Fitness F>
const auto &analyzer<I, F>::crossover_types(population_uid g) const
{
  const auto *ptr(group(g));
  assert(ptr);

  return ptr->crossover_type;
}

///
/// \param[in] g a population / subpopulation
/// \return      statistics about the crossover operators used in group `g`
///
template<Individual I, Fitness F>
template<Population P>
const auto &analyzer<I, F>::crossover_types(const P &g) const
{
  return crossover_types(g.uid());
}

///
/// \return statistics about the age distribution of the individuals (entire
///         population)
///
template<Individual I, Fitness F>
distribution<double> analyzer<I, F>::age_dist() const
{
  distribution<double> ret;
  for (const auto &gs : group_stat_)
    ret.merge(gs.age);

  return ret;
}

///
/// \param[in] g the UID of a population / subpopulation
/// \return      statistics about the age distribution of individuals in group
///              `g`
///
template<Individual I, Fitness F>
const distribution<double> &analyzer<I, F>::age_dist(population_uid g) const
{
  const auto *ptr(group(g));
  assert(ptr);

  return ptr->age;
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
  return age_dist(g.uid());
}

///
/// \return statistics about the fitness distribution of the individuals
///         (entire population)
///
template<Individual I, Fitness F>
distribution<F> analyzer<I, F>::fit_dist() const
{
  distribution<F> ret;
  for (const auto &gs : group_stat_)
    ret.merge(gs.fitness);

  return ret;
}

///
/// \param[in] g the UID of a population / subpopulation
/// \return      statistics about the fitness distribution of individuals in
///              group `g`
///
template<Individual I, Fitness F>
const distribution<F> &analyzer<I, F>::fit_dist(population_uid g) const
{
  const auto *ptr(group(g));
  assert(ptr);

  return ptr->fitness;
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
  return fit_dist(g.uid());
}

///
/// \return statistic about the length distribution of the individuals
///         (entire population)
///
template<Individual I, Fitness F>
distribution<double> analyzer<I, F>::length_dist() const
{
  distribution<double> ret;
  for (const auto &gs : group_stat_)
    ret.merge(gs.length);

  return ret;
}

///
/// \param[in] g the UID a population / subpopulation
/// \return      statistics about the length distribution of individuals in
///              group `g`
///
template<Individual I, Fitness F>
const distribution<double> &analyzer<I, F>::length_dist(population_uid g) const
{
  const auto *ptr(group(g));
  assert(ptr);

  return ptr->length;
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
  return length_dist(g.uid());
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
/// \param[in] uid a group of the population
///
/// The optional `uid` parameter is used to split information.
///
template<Individual I, Fitness F>
void analyzer<I, F>::add(const I &ind, const F &f, population_uid uid)
{
  if (auto *selected = group(uid))
    selected->add(ind, f);
  else
  {
    group_stat_.emplace_back(uid);
    group_stat_.back().add(ind, f);
  }
}

#endif  // include guard
