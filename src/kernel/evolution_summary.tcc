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

#if !defined(ULTRA_EVOLUTION_SUMMARY_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_SUMMARY_TCC)
#define      ULTRA_EVOLUTION_SUMMARY_TCC

///
/// Resets summary informations.
///
template<Individual I, Fitness F>
void summary<I, F>::clear()
{
  *this = summary<I, F>();
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
bool summary<I, F>::load(std::istream &in, const problem &p)
{
  summary tmp_summary;

  if (!tmp_summary.status.load(in, p))
    return false;

  if (!tmp_summary.score.load(in))
    return false;

  if (int ms; !(in >> ms))
    return false;
  else
    tmp_summary.elapsed = std::chrono::milliseconds(ms);

  if (!(in >> tmp_summary.gen >> tmp_summary.last_imp))
    return false;

  *this = tmp_summary;
  return true;
}

///
/// Saves the object into a stream.
///
/// \param[out] out output stream
/// \return         `true` if summary was saved correctly
///
template<Individual I, Fitness F>
bool summary<I, F>::save(std::ostream &out) const
{
  if (!status.save(out))
    return false;

  // analyzer `az` doesn't need to be saved: it'll be recalculated at the
  // beginning of evolution.

  if (!score.save(out))
    return false;

  out << elapsed.count() << ' ' << gen << ' ' << last_imp << '\n';

  return out.good();
}

#endif  // include guard
