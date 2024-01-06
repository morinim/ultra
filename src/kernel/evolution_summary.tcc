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

template<Individual I, Fitness F>
void summary<I, F>::update_best(const I &prg, const F &fit)
{
  last_imp = gen;
  best.solution = prg;
  best.score.fitness = fit;
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
  unsigned known_best(false);
  if (!(in >> known_best))
    return false;

  summary tmp_summary;
  if (known_best)
  {
    if (!tmp_summary.best.solution.load(in, p.sset))
      return false;

    if (!tmp_summary.best.score.load(in))
      return false;
  }

  if (int ms; !(in >> ms))
    return false;
  else
    tmp_summary.elapsed = std::chrono::milliseconds(ms);

  if (!(in >> tmp_summary.mutations  >> tmp_summary.crossovers
           >> tmp_summary.gen >> tmp_summary.last_imp))
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
  // analyzer `az` doesn't need to be saved: it'll be recalculated at the
  // beginning of evolution.

  if (best.solution.empty())
    out << "0\n";
  else
  {
    out << "1\n";
    if (!best.solution.save(out))
      return false;
    if (!best.score.save(out))
      return false;
  }

  out << elapsed.count() << ' ' << mutations << ' ' << crossovers << ' '
      << gen << ' ' << last_imp << '\n';

  return out.good();
}

#endif  // include guard
