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
  ultraDEBUG << "Creating a new instance of test_evaluator " << et;
}

///
/// \param[in] prg a program (individual/team)
/// \return        fitness value for individual `prg`
///
/// Depending on the type of test_evaluator returns:
/// - a distinct, random, time-invariant fitness value for each semantically
///   equivalent `prg` (realistic);
/// - a random, time-variant fitness value for `prg` (random);
/// - a fixed, time-invariant fitness value for every individual of the
///   population (fixed);
/// - the age of the individual (age). This is useful when using a static
///   layered_population produced via `make_debug_population` (every individual
///   has a distinct fitness).
///
template<Individual I>
double test_evaluator<I>::operator()(const I &prg) const
{
  if (delay_ > std::chrono::milliseconds{0})
    std::this_thread::sleep_for(delay_);

  switch (et_)
  {
  case test_evaluator_type::fixed:
    return 0.0;

  case test_evaluator_type::random:
    return random::sup<double>(1000000);

  case test_evaluator_type::age:
    return static_cast<double>(prg.age());

  default:  // test_evaluator_type::realistic
  {
    const auto signature(prg.signature());
    return static_cast<double>(static_cast<std::uint32_t>(signature.data[0]));
  }
  }
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
