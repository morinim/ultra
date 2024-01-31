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
evolution_status<I, F>::evolution_status(const scored_individual<I, F> &si)
  : best_(si)
{
  Expects(!si.empty());
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

  std::atomic_bool flag(false);

  {
    std::shared_lock lock(*pmutex_);
    flag = si > best_;

    // By default an empty `scored_individual` has a the lowest possible
    // fitness so it'll be immediately replaced.
  }

  if (!flag)
    return false;

  std::lock_guard lock(*pmutex_);
  if (si > best_)
  {
    best_ = si;
    return true;
  }

  return false;
}

///
/// \return a copy of the best scored individual found so far
///
template<Individual I, Fitness F>
scored_individual<I, F> evolution_status<I, F>::best() const
{
  std::shared_lock lock(*pmutex_);
  return best_;
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

  std::uintmax_t tmp_mut;
  static_assert(std::is_same_v<typename decltype(mutations)::value_type,
                               decltype(tmp_mut)>);
  if (!(in >> tmp_mut))
    return false;

  std::uintmax_t tmp_cross;
  static_assert(std::is_same_v<typename decltype(crossovers)::value_type,
                               decltype(tmp_cross)>);
  if (!(in >> tmp_cross))
    return false;

  {
    std::lock_guard lock(*pmutex_);
    best_ = tmp_si;
    mutations = tmp_mut;
    crossovers = tmp_cross;
  }

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
  std::shared_lock lock(*pmutex_);
  {
    if (!best_.save(out))
      return false;

    out << mutations << ' ' << crossovers << '\n';
  }

  return out.good();
}

#endif  // include guard
