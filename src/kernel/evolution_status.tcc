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

#if !defined(ULTRA_EVOLUTION_STATUS_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_STATUS_TCC)
#define      ULTRA_EVOLUTION_STATUS_TCC

template<Individual I, Fitness F>
evolution_status<I, F>::evolution_status(const unsigned *gen,
                                         const global_update_f &f)
  : update_overall_best_(f), generation_(gen)
{
}

///
/// Update, when appropriate, the best known individual.
///
/// \param[in] si a scored individual
/// \return       `true` if best individual has been updated
///
template<Individual I, Fitness F>
bool evolution_status<I, F>::update_if_better(const scored_individual<I, F> &si)
{
  Expects(!si.empty());

  const bool update(si > best_);

  if (update)
  {
    best_ = si;

    if (update_overall_best_)
      update_overall_best_(best_);
  }

  return update;
}

///
/// \return a copy of the best scored individual found so far
///
template<Individual I, Fitness F>
scored_individual<I, F> evolution_status<I, F>::best() const
{
  return best_;
}

template<Individual I, Fitness F>
unsigned evolution_status<I, F>::generation() const
{
  return generation_ ? *generation_ : std::numeric_limits<unsigned>::max();
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
bool evolution_status<I, F>::load(std::istream &in, const problem &p)
{
  scored_individual<I, F> tmp_si;
  if (!tmp_si.load(in, p))
    return false;

  best_ = tmp_si;

  return true;
}

///
/// Saves the object into a stream.
///
/// \param[out] out output stream
/// \return         `true` if the object was saved correctly
///
template<Individual I, Fitness F>
bool evolution_status<I, F>::save(std::ostream &out) const
{
  if (!best_.save(out))
    return false;

  // `generation_` is just a reference, no need to save it.

  return out.good();
}

#endif  // include guard
