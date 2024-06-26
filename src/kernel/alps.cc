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

#include <limits>

#include "kernel/alps.h"

namespace ultra::alps
{

///
/// Given a layer index returns the associated maximum allowed age of the
/// members.
///
/// \param[in] l layer index
/// \return      the maximum allowed age for an individual in layer `l`
///
/// Parameters from the environment:
/// - `age_gap`
///
unsigned parameters::max_age(std::size_t l) const
{
  // A polynomial aging scheme.
  switch (l)
  {
  case 0:   return age_gap;
  case 1:   return age_gap + age_gap;
  default:  return l * l * age_gap;
  }

  // A linear aging scheme.
  // return age_gap * (l + 1);

  // An exponential aging scheme.
  // switch (l)
  // {
  // case 0:  return age_gap;
  // case 1:  return age_gap + age_gap;
  // default:
  // {
  //   auto k(4);
  //   for (unsigned i(2); i < layer; ++i)
  //     k *= 2;
  //   return k * age_gap;
  // }

  // Fibonacci aging scheme.
  // auto num1(age_gap), num2(age_gap);
  // while (num2 <= 2)
  // {
  //   auto num3(num2);
  //   num2 += num1;
  //   num1 = num3;
  // }
  //
  // if (l == 1)
  //   return num1 + num2 - 1;
  //
  // for (unsigned i(1); i <= l; ++i)
  // {
  //   auto num3(num2);
  //   num2 += num1 -1;
  //   num1 = num3;
  // }
  // return num2;
}

///
/// \param[in] l      a specific layer
/// \param[in] layers total number of layers the population is structured on
/// \return           the maximum allowed age for an individual in layer `l`.
///                   For individuals in the last layer there isn't a age limit
///
unsigned parameters::max_age(std::size_t l, std::size_t layers) const
{
  Expects(l < layers);

  return l + 1 == layers ? std::numeric_limits<unsigned>::max()
                         : max_age(l);
}

}  // namespace ultra::alps
