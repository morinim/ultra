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
/// \return a valid starting evolution status to be used by an evolution
///         strategy
///
/// A starting evolution status is an "empty" `evolution_status` object with
/// reference to `this` object correctly set, i.e.:
/// - the callback function used to keep up to date the best individual found;
/// - the current generation.
///
template<Individual I, Fitness F>
evolution_status<I, F> summary<I, F>::starting_status()
{
  return evolution_status<I, F>(&generation,
                                [this](scored_individual<I, F> prg)
                                {
                                  update_if_better(prg);
                                });
}

///
/// Keeps updated the best individual found so far.
///
/// \param[in] prg candidate new best scored individual
///
template<Individual I, Fitness F>
void summary<I, F>::update_if_better(scored_individual<I, F> prg)
{
  best_.write([&prg](auto &best)
  {
    if (prg > best)
      best = prg;
  });
}

///
/// \return best scored individul found so far
///
template<Individual I, Fitness F>
scored_individual<I, F> summary<I, F>::best() const
{
  return best_.read([](const auto &best) { return best; });
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

  std::chrono::milliseconds tmp_elapsed;
  if (int ms; !(in >> ms))
    return false;
  else
    tmp_elapsed = std::chrono::milliseconds(ms);

  unsigned tmp_generation;
  if (!(in >> tmp_generation))
    return false;

  model_measurements<F> tmp_score;
  if (!tmp_score.load(in))
    return false;

  scored_individual<I, F> tmp_best;
  if (!tmp_best.load(in, p))
    return false;

  score = tmp_score;
  elapsed = tmp_elapsed;
  generation = tmp_generation;
  best_ = tmp_best;

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
  // Since `status` depends on `generaiton`, saving `generation` before
  // `status` is very important.
  out << elapsed.count() << ' ' << generation << '\n';

  // analyzer `az` doesn't need to be saved: it'll be recalculated at the
  // beginning of evolution.

  if (!score.save(out))
    return false;

  if (!best_.read([&out](const auto &best) { return best.save(out); }))
    return false;

  return out.good();
}

#endif  // include guard
