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

#if !defined(ULTRA_POPULATION_H)
#define      ULTRA_POPULATION_H

#include "kernel/individual.h"
#include "kernel/random.h"

#include <concepts>
#include <ranges>
#include <shared_mutex>

namespace ultra
{

///
/// The numerical type used for population unique ID.
///
using population_uid = unsigned;

template<class P>
concept RandomAccessIndividuals =
  std::ranges::random_access_range<P>
  && Individual<std::ranges::range_value_t<P>>;

template<class P>
concept Population =
  std::ranges::range<P>
  && Individual<std::ranges::range_value_t<P>>;

template<class P>
concept LayeredPopulation = Population<P> && requires(const P &p)
{
  { p.layers() } -> std::integral;
  { p.range_of_layers() } -> std::ranges::range;
};

template<class P>
concept SizedRandomAccessPopulation =
  Population<P>
  && std::ranges::sized_range<P>
  && std::ranges::random_access_range<P>
  && requires(const P &p)
{
  typename P::coord;
  p[typename P::coord()];
};

template<class P>
concept PopulationWithMutex =
  SizedRandomAccessPopulation<P> && requires(const P &p)
{
  p.mutex();
};

namespace random
{

template<SizedRandomAccessPopulation P>
[[nodiscard]] typename P::coord coord(const P &p)
{
  Expects(p.size() > 0);
  return static_cast<typename P::coord>(sup(p.size()));
}

///
/// Returns a coordinate sampled from a position's neighbourhood.
///
/// \param[in] p         a population
/// \param[in] i         base coordinate
/// \param[in] mate_zone neighbourhood radius (must be non-zero)
/// \return              the sampled coordinate
///
/// \pre i < p.size()
/// \pre mate_zone > 0
///
/// The neighbourhood is interpreted on a circular (ring) topology. For large
/// neighbourhoods (`mate_zone >= p.size() / 2`) the sampling degenerates to a
/// uniform choice over the whole population.
///
template<SizedRandomAccessPopulation P>
[[nodiscard]] typename P::coord
coord(const P &p, std::size_t i, std::size_t mate_zone)
{
  Expects(i < p.size());
  Expects(mate_zone);  // non-zero after auto-tune

  return static_cast<typename P::coord>(random::ring(i, mate_zone, p.size()));
}

///
/// \param[in] p a population
/// \return      a random individual of the population
///
template<PopulationWithMutex P>
[[nodiscard]] std::ranges::range_value_t<P> individual(const P &p)
{
  std::shared_lock lock(p.mutex());
  return p[random::coord(p)];
}

}  // namespace random

}  // namespace ultra

#endif  // include guard
