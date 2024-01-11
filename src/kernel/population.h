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

#include <shared_mutex>

#include "kernel/individual.h"
#include "kernel/random.h"

namespace ultra
{

template<class P>
concept SizedRangeOfIndividuals = requires(const P &p)
{
  requires std::ranges::sized_range<P>;
  requires Individual<std::ranges::range_value_t<P>>;
};

template<class P>
concept Population = requires(const P &p)
{
  requires SizedRangeOfIndividuals<P>;
  typename P::value_type;
  requires Individual<typename P::value_type>;
};

template<class P>
concept LayeredPopulation = Population<P> && requires(const P &p)
{
  p.layers();
};

template<class P>
concept RandomAccessPopulation = requires(const P &p)
{
  requires Population<P>;
  typename P::coord;
  p[typename P::coord()];
};

template<class P>
concept PopulationWithMutex = RandomAccessPopulation<P> && requires(const P &p)
{
  p.mutex();
};

namespace random
{

template<RandomAccessPopulation P>
[[nodiscard]] std::size_t coord(const P &l)
{
  return sup(l.size());
}

template<RandomAccessPopulation P>
[[nodiscard]] std::size_t
coord(const P &l, std::size_t i, std::size_t mate_zone)
{
  return random::ring(i, mate_zone, l.size());
}

///
/// \param[in] p a population
/// \return      a random individual of the population
///
template<PopulationWithMutex P>
[[nodiscard]] typename P::value_type individual(const P &p)
{
  std::shared_lock lock(p.mutex());
  return p[random::coord(p)];
}

}  // namespace random

}  // namespace ultra

#endif  // include guard
