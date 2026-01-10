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

///
/// Constructs a test evaluator with the specified evaluation strategy.
///
/// \param[in] et the evaluation strategy to use
///
template<Individual I>
test_evaluator<I>::test_evaluator(test_evaluator_type et) : et_(et)
{
  ultraDEBUG << "Creating a new instance of test_evaluator " << et;
}

///
/// Evaluates the fitness of an individual.
///
/// \param[in] prg the individual whose fitness is to be computed
/// \return        the fitness value
///
/// The returned value depends on the evaluator type selected at construction:
/// - a fixed, time-invariant value for all individuals (`fixed`);
/// - a random, time-variant value (`random`);
/// - a deterministic value derived from the individual's signature
///   (`realistic`);
/// - the age of the individual (`age`).
///
/// If a delay has been configured via `delay`, the evaluation blocks for the
/// specified duration before computing the fitness.
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
/// Adds a fixed delay to each evaluation.
///
/// \param[in] ms the delay duration in applied to every evaluation
///
/// This is useful for simulating computationally expensive fitness functions
/// when testing scheduling, parallelism, or caching behaviour.
///
template<Individual I>
void test_evaluator<I>::delay(std::chrono::milliseconds ms) noexcept
{
  delay_ = ms;
}

#endif  // include guard
