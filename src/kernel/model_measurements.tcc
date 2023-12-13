/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_MODEL_MEASUREMENTS_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_MODEL_MEASUREMENTS_TCC)
#define      ULTRA_MODEL_MEASUREMENTS_TCC

template<Fitness F>
model_measurements<F>::model_measurements(const F &f, double a)
  : fitness(f), accuracy(a)
{
  Expects(0.0 <= accuracy && accuracy <= 1.0);
}

///
/// Loads the object from a stream.
///
/// \param[in] is input stream
/// \return       `true` if the object loaded correctly
///
/// \note
/// If the load operation isn't successful the current object isn't changed.
///
template<Fitness F>
bool model_measurements<F>::load(std::istream &is)
{
  model_measurements tmp;

  if (!ultra::load(is, &tmp.fitness))
    return false;

  if (!load_float_from_stream(is, &tmp.accuracy))
    return false;

  *this = tmp;

  return true;
}

///
/// Saves the object into a stream.
///
/// \param[out] os output stream
/// \return        `true` if summary was saved correctly
///
template<Fitness F>
bool model_measurements<F>::save(std::ostream &os) const
{
  if (!ultra::save(os, fitness))
    return false;
  if (!save_float_to_stream(os, accuracy))
    return false;

  os << '\n';

  return os.good();
}

#endif  // include guard
