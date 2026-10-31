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

#if !defined(ULTRA_DE_NUMERICAL_REFINER_H)
#define      ULTRA_DE_NUMERICAL_REFINER_H

#include "kernel/de/search.h"
#include "kernel/decision_vector.h"
#include "kernel/refiner.h"

#include <algorithm>

namespace ultra::de
{

struct numerical_refinement_backend
{
  ///
  /// Refines the tunable scalar parameters of `ind`.
  ///
  /// The optimiser extracts a decision vector from `ind`, performs numerical
  /// optimisation over a bounded region around the current parameter values,
  /// and writes the best solution back into `ind`.
  ///
  /// If `ind` exposes no optimisable parameters, this function has no effect.
  ///
  template<Evaluator E>
  requires NumericalOptimisable<evaluator_individual_t<E>>
  std::optional<evaluator_fitness_t<E>> operator()(
    evaluator_individual_t<E> &ind, const E &eva,
    const parameters::refinement_parameters &params) const
  {
    Expects(params.de.min_radius > 0.0);
    Expects(params.de.rel_radius >= 0.0);

    const auto eval([&eva](const auto &cand)
    {
      if constexpr (requires { eva.core(); })
        return eva.core()(cand);
      else
        return eva(cand);
    });

    auto dv(extract_decision_vector(ind));

    if (dv.empty())
      return {};

    std::vector<interval<double>> ranges;
    ranges.reserve(dv.size());

    for (const auto val : dv.values)
    {
      const auto delta(std::max(std::abs(val) * params.de.rel_radius,
                                params.de.min_radius));
      ranges.emplace_back(val - delta, val + delta);
    }

    problem de_prob(ranges);
    de_prob.params.population.init_subgroups = 1;
    de_prob.params.population.individuals = std::max(
      params.de.individuals, parameters::de_parameters::min_individuals);
    de_prob.params.evolution.generations  = params.de.generations;

    // Reusable working buffer to avoid allocations in the evaluation loop.
    auto trial(ind);

    const auto de_eva([&](const de::individual &vec)
    {
      std::ranges::copy(vec, dv.values.begin());
      trial.apply_decision_vector(dv);

      return eval(trial);
    });

    const auto base(eval(ind));

    search de_search(de_prob, de_eva);
    de_search.messages(params.messages);
    const auto res(de_search.run());

    if (!res.runs() || res.best_measurements().fitness <= base)
      return {};

    std::ranges::copy(res.best_individual(), dv.values.begin());
    ind.apply_decision_vector(dv);

    return res.best_measurements().fitness;
  }
};

/// Convenience API for DE-based numerical refinement.
template<Evaluator E>
requires NumericalOptimisable<evaluator_individual_t<E>>
std::optional<evaluator_fitness_t<E>> optimise(const refiner &opt,
                                               evaluator_individual_t<E> &ind,
                                               const E &eva)
{
  return opt.optimise(ind, eva, numerical_refinement_backend());
}

}  // namespace ultra::de

#endif  // include guard
