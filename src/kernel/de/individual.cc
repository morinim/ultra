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

#include "kernel/de/individual.h"
#include "kernel/print_internal.h"
#include "kernel/random.h"

#include "utility/misc.h"

#include <algorithm>

namespace ultra::de
{

///
/// Differential evolution crossover.
///
/// \param[in] p crossover probability (`parameters.evolution.p_cross`)
/// \param[in] f scaling factor interval (`parameters.de.weight`)
/// \param[in] a base vector
/// \param[in] b first vector used for calculating the difference vector
/// \param[in] c second vector used for calculating the difference vector
/// \return      the offspring / trial vector
///
/// First the search direction is defined by calculating a *difference vector*
/// between the pair of vectors `b` and `c` (usually chosen at random from the
/// population). This difference vector is scaled by using the *scale factor*
/// `F`.
/// This way no separate probability distribution has to be used which makes
/// the scheme completely self-organizing.
///
/// The scaled difference vector is then added to a third vector `a`, called
/// the *base* vector. As a result a new vector is obtained, known as the
/// *mutant* or *donor* vector:
///
///     m = a + F * (b - c)
///
/// The mutation strategy used is named `DE/rand/1`.
/// Now the *offspring*, also called *trial*, vector is generated as follows:
///
///     offspring = crossover(this, m)
///
/// The mutant vector is recombined, based on a user defined parameter, called
/// *crossover probability*, with `this` (the *target* / *parent* vector).
/// The offspring gets **at least** one component of the mutant vector (the
/// last).
/// This scheme is called *binomial*.
///
/// `b` and `c` are used for mutation, `this` and `m` for crossover.
///
individual individual::crossover(double p, const interval<double> &f,
                                 const individual &a,
                                 const individual &b,
                                 const individual &c) const
{
  Expects(0.0 <= p && p <= 1.0);

  const auto ps(parameters());
  Expects(ps == a.parameters());
  Expects(ps == b.parameters());
  Expects(ps == c.parameters());

  // The weighting factor is randomly selected from an interval for each
  // difference vector (a technique called dither). Dither improves convergence
  // behaviour significantly, especially for noisy objective functions.
  const auto rf(random::element(f));

  auto ret(a);

  const auto last(ps - 1);
  for (std::size_t i(0); i < last; ++i)
    if (random::boolean(p))  // binomial scheme
      ret.genome_[i] += rf * (b[i] - c[i]);
    else
      ret.genome_[i] = operator[](i);

  // Last element is replaced for certain.
  ret.genome_[last] += rf * (b[last] - c[last]);

  ret.set_if_older_age(std::max({age(), a.age()}));

  ret.signature_ = ret.hash();
  Ensures(ret.is_valid());
  return ret;
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs` (taxicab / L1 distance)
///
double distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0,
                            std::plus{},
                            [](auto a, auto b) { return std::abs(a - b); });
}

void individual::print_impl(std::ostream &s, out::print_format_t format) const
{
  internal::print_linear(s, *this, format, "de::individual");
}

}  // namespace ultra::de
