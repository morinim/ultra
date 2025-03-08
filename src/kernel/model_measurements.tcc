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
/// \return `true` if all the fields are empty
///
template<Fitness F>
bool model_measurements<F>::empty() const noexcept
{
  return !fitness.has_value() && !accuracy.has_value();
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        a partial ordering result
///
/// The comparison criterium is Pareto dominance
/// (https://en.wikipedia.org/wiki/Pareto_efficiency)
///
template<Fitness F>
auto operator<=>(const model_measurements<F> &lhs,
                 const model_measurements<F> &rhs) noexcept
{
  if (lhs.fitness <= rhs.fitness && lhs.accuracy <= rhs.accuracy
      && (lhs.fitness < rhs.fitness || lhs.accuracy < rhs.accuracy))
    return std::partial_ordering::less;
  if (lhs.fitness >= rhs.fitness && lhs.accuracy >= rhs.accuracy
      && (lhs.fitness > rhs.fitness || lhs.accuracy > rhs.accuracy))
    return std::partial_ordering::greater;
  if (lhs == rhs)
    return std::partial_ordering::equivalent;

  return std::partial_ordering::unordered;  // {12, 50} <=> {10, 60}
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

  unsigned has_fitness;
  if (!(is >> has_fitness))
    return false;
  if (has_fitness)
  {
    if (F tmp_fitness; !ultra::load(is, &tmp_fitness))
      return false;
    else
      tmp.fitness = tmp_fitness;
  }

  unsigned has_accuracy;
  if (!(is >> has_accuracy))
    return false;
  if (has_accuracy)
  {
    if (double tmp_accuracy; !load_float_from_stream(is, &tmp_accuracy))
      return false;
    else
      tmp.accuracy = tmp_accuracy;
  }

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
  if (fitness.has_value())
  {
    os << "1 ";
    if (!ultra::save(os, *fitness))
      return false;
  }
  else
    os << "0";

  os << '\n';

  if (accuracy.has_value())
  {
    os << "1 ";
    if (!save_float_to_stream(os, *accuracy))
      return false;
  }
  else
    os << "0";

  os << '\n';

  return os.good();
}

#endif  // include guard
