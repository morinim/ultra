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

#if !defined(ULTRA_SCORED_INDIVIDUAL_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SCORED_INDIVIDUAL_TCC)
#define      ULTRA_SCORED_INDIVIDUAL_TCC

///
/// Builds a new individual/fitness pair.
///
/// \param[in] i an individual (can be empty)
/// \param[in] f a fitness
///
template<Individual I, Fitness F>
scored_individual<I, F>::scored_individual(const I &i, const F &f)
  : ind(i), fit(f)
{
  Expects(!i.empty());
}

///
/// \return `true` if the `ind` component is empty
///
template<Individual I, Fitness F>
bool scored_individual<I, F>::empty() const noexcept
{
  return ind.empty();
}

///
/// Orders two scored individuals by fitness.
///
/// \param lhs first scored individual
/// \param rhs second scored individual
/// \return    the ordering of `lhs.fit` and `rhs.fit`
///
/// The comparison is based **only on the fitness value** and is intended for
/// ranking and selection purposes (e.g. best/worst individual).
///
/// \note
/// This operator deliberately provides *ordering only*. Equality
/// (`operator==`) is not defined, because fitness values are typically
/// floating-point and exact equality would be semantically misleading.
///
/// As a consequence, `scored_individual` does not model a totally ordered type
/// and cannot be used with default `std::ranges::less`. Algorithms requiring
/// ordering should supply an explicit comparator or projection.
///
template<Individual I, Fitness F>
auto operator<=>(const scored_individual<I, F> &lhs,
                 const scored_individual<I, F> &rhs) noexcept
{
  return lhs.fit <=> rhs.fit;
}

///
/// Loads the object from a stream.
///
/// \param[in] in input stream
/// \param[in] p  active problem
/// \return       `true` if the object loaded correctly
///
/// \note
/// If the load operation isn't successful the current object isn't changed.
///
template<Individual I, Fitness F>
bool scored_individual<I, F>::load(std::istream &in, const problem &p)
{
  unsigned known_best(false);
  if (!(in >> known_best))
    return false;

  scored_individual tmp;
  if (known_best)
  {
    if (!tmp.ind.load(in, p.sset))
      return false;

    if (!ultra::load(in, &tmp.fit))
      return false;
  }

  *this = tmp;
  return true;
}

///
/// Saves the object into a stream.
///
/// \param[out] out output stream
/// \return         `true` if the object was saved correctly
///
template<Individual I, Fitness F>
bool scored_individual<I, F>::save(std::ostream &out) const
{
  if (empty())
    out << "0\n";
  else
  {
    out << "1\n";
    if (!ind.save(out))
      return false;
    if (!ultra::save(out, fit))
      return false;
  }

  return out.good();
}

#endif  // include guard
