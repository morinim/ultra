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

#include "kernel/ga/individual.h"
#include "kernel/print_internal.h"
#include "kernel/random.h"

#include "utility/misc.h"

#include <algorithm>

namespace ultra::ga
{

///
/// Mutates the current individual.
///
/// \param[in] prb         the current problem
/// \param[in] temperature mutation intensity (`1.0` preserves the configured
///                        probability)
/// \return                number of mutations performed
///
/// \note
/// External parameters: `evolution.p_mutation`
///
unsigned individual::mutation(const problem &prb, double temperature)
{
  Expects(in_0_1(prb.params.evolution.p_mutation));
  Expects(temperature >= 0.0);

  const double pgm(
    1.0 - std::pow(1.0 - prb.params.evolution.p_mutation, temperature));

  unsigned n(0);

  const auto ps(parameters());
  for (symbol::category_t c(0); c < ps; ++c)
    if (random::boolean(pgm))
      if (const auto g(std::get<D_INT>(prb.sset.roulette_terminal(c)));
          g != genome_[c])
      {
        ++n;
        genome_[c] = g;
      }

  if (n)
    signature_ = hash();

  Ensures(is_valid());
  return n;
}

///
/// Calculates the Hamming distance between two individuals.
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs` (the number of different genes)
///
/// \relates ga::individual
///
unsigned distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  return hamming_distance(lhs, rhs);
}

///
/// Two points crossover.
///
/// \param[in] lhs first parent
/// \param[in] rhs second parent
/// \return        crossover children (we only generate a single offspring)
///
/// We randomly select two loci (common crossover points). The offspring is
/// created with genes from the `rhs` parent before the first crossover
/// point and after the second crossover point; genes between crossover
/// points are taken from the `lhs` parent.
///
/// \note Parents must have the same size.
///
/// \relates ga::individual
///
individual crossover(const problem &,
                     const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  const auto ps(lhs.parameters());
  const auto cut1(random::sup(ps - 1));
  const auto cut2(random::between(cut1 + 1, ps));

  auto ret(rhs);
  for (auto i(cut1); i < cut2; ++i)
    ret.genome_[i] = lhs[i];  // not using `operator[](std::size_t)` to avoid
                              // multiple signature resets.

  ret.set_if_older_age(lhs.age());
  ret.signature_ = ret.hash();

  Ensures(ret.is_valid());
  return ret;
}

void individual::print_impl(std::ostream &s, out::print_format_t format) const
{
  internal::print_linear(s, *this, format, "ga::individual");
}

}  // namespace ultra::ga
