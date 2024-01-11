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

#if !defined(ULTRA_LAYERED_POPULATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_LAYERED_POPULATION_COORD_TCC)
#define      ULTRA_LAYERED_POPULATION_COORD_TCC

///
/// Holds the coordinates of an individual in a population.
///
template<Individual I>
struct layered_population<I>::coord
{
  std::size_t layer;
  std::size_t index;

  [[nodiscard]] bool operator==(const coord &rhs) const = default;
  [[nodiscard]] bool operator<(const coord &rhs) const
  {
    return layer < rhs.layer
           || (layer == rhs.layer && index < rhs.index);
  }
};  // struct coord

#endif  // include guard
