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

#if !defined(ULTRA_EVOLUTION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_EVOLUTION_TCC)
#define      ULTRA_EVOLUTION_TCC

///
/// \param[in] strategy evolution strategy to be used
///
template<Strategy S>
evolution<S>::evolution(const S &strategy)
  : sum_(), pop_(strategy.problem()), es_(strategy)
{
  Ensures(is_valid());
}

///
/// \return `true` when evolution should be interrupted
///
template<Strategy S>
bool evolution<S>::stop_condition() const
{
  //const auto generations(pop_.problem().env.generations);
  //Expects(generations);

  // Check the number of generations.
  //if (s.gen > generations)
  //  return true;

  //if (term::user_stop())
  //  return true;

  // Check strategy specific stop conditions.
  //return es_.stop_condition();

  return sum_.generation > 1000;
}

///
/// The evolutionary core loop.
///
/// \return a partial summary of the search (see notes)
///
/// \note
/// The return value is a partial summary: the `mode_measurement` section is
/// only partially filled (fitness) since many metrics are expensive to
/// calculate and not always significative  (e.g. f1-score for a symbolic
/// regression problem). The `src_search` class has a simple scheme to
/// request the computation of additional metrics.
///
template<Strategy S>
summary<typename S::individual_t, typename S::fitness_t>
evolution<S>::run()
{
  timer measure;

  const auto evolve_layer(
    [&](auto layer_iter)
    {
      auto evolve(es_.operations(pop_, layer_iter, sum_.starting_status()));

      for (auto cycles(layer_iter->size()); cycles; --cycles)
        evolve();
    });

  es_.init(pop_);

  for (; !stop_condition();  ++sum_.generation)
  {
    const auto range(pop_.range_of_layers());

    {
      std::vector<std::jthread> threads;

      for (auto l(range.begin()); l != range.end(); ++l)
        threads.emplace_back(evolve_layer, l);
    }

    sum_.az = analyze(pop_, es_.evaluator());
    es_.after_generation(sum_.generation, pop_, sum_.az);
  }

  sum_.elapsed = measure.elapsed();

  return sum_;
}

template<Strategy S>
bool evolution<S>::is_valid() const
{
  return true;
}

#endif  // include guard
