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

#if !defined(ULTRA_EVALUATOR_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVALUATOR_TCC)
#define      ULTRA_EVALUATOR_TCC

template<Individual I>
test_evaluator<I>::test_evaluator(test_evaluator_type et) : et_(et)
{
}

///
/// \param[in] prg a program (individual/team)
/// \return        fitness value for individual `prg`
///
/// Depending on the type of test_evaluator returns:
/// - a random, time-invariant fitness value for `prg`;
/// - a fixed, time-invariant fitness value for every individual of the
///   population;
/// - a distinct, time-invariant fitness value for each `prg`.
///
template<Individual I>
double test_evaluator<I>::operator()(const I &prg) const
{
  if (delay_ > std::chrono::milliseconds{0})
    std::this_thread::sleep_for(delay_);

  if (et_ == test_evaluator_type::fixed)
    return 0.0;

  std::lock_guard guard(mutex_);
  auto it(std::ranges::find(buffer_, prg));
  if (it == buffer_.end())
  {
    buffer_.push_back(prg);
    it = std::prev(buffer_.end());
  }

  const auto dist(std::distance(buffer_.begin(), it));

  if (et_ == test_evaluator_type::distinct)
    return static_cast<double>(dist);

  assert(et_ == test_evaluator_type::random);
  static random::engine_t e;
  e.seed(dist);
  return static_cast<double>(e());
}

///
/// Add a fixed delay for every call to the evaluator.
///
/// \param[in] ms a delay
///
/// This is useful for simulating complex evaluators.
///
template<Individual I>
void test_evaluator<I>::delay(std::chrono::milliseconds ms)
{
  delay_ = ms;
}

#endif  // include guard
