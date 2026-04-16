/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_NUMERICAL_OPTIMISER_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_NUMERICAL_OPTIMISER_TCC)
#define      ULTRA_NUMERICAL_OPTIMISER_TCC

template<Evaluator E>
void numerical_optimiser::optimise(gp::individual &p, const E &eva) const
{
  const auto dv(extract_decision_vector(p));

  if (dv.empty())
    return;

  std::vector<interval<double>> ranges;
  ranges.reserve(dv.size());

  for (std::size_t i(0); i < dv.size(); ++i)
  {
    const auto delta(std::max(std::abs(dv.values[i]) * params_.rel_radius,
                              params_.min_radius));
    ranges.emplace_back(dv.values[i] - delta, dv.values[i] + delta);
  }

  de::problem de_prob(ranges);
  de_prob.params.population.individuals = params_.individuals;
  de_prob.params.evolution.generations  = params_.generations;

  const auto de_eva([&](const de::individual &vec)
  {
    auto trial(p);
    trial.apply_decision_vector(decision_vector{vec, dv.coords});

    return eva(trial);
  });

  de::search search(de_prob, de_eva);
  const auto res(search.run());

  p.apply_decision_vector(decision_vector{res.best_individual(), dv.coords});
}

#endif  // include guard
