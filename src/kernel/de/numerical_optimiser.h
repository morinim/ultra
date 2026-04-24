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

#if !defined(ULTRA_DE_NUMERICAL_OPTIMISER_H)
#define      ULTRA_DE_NUMERICAL_OPTIMISER_H

#include "kernel/de/search.h"
#include "kernel/numerical_optimiser.h"

namespace ultra::de
{

struct refinement_backend
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
    const parameters::numerical_optimisation_parameters &params) const
  {
    const auto dv(extract_decision_vector(ind));
    using dv_t = decision_vector_t<evaluator_individual_t<E>>;

    if (dv.empty())
      return {};

    std::vector<interval<double>> ranges;
    ranges.reserve(dv.size());

    for (std::size_t i(0); i < dv.size(); ++i)
    {
      const auto delta(std::max(std::abs(dv.values[i]) * params.rel_radius,
                                params.min_radius));
      ranges.emplace_back(dv.values[i] - delta, dv.values[i] + delta);
    }

    problem de_prob(ranges);
    de_prob.params.population.individuals = params.individuals;
    de_prob.params.evolution.generations  = params.generations;

    const auto de_eva([&](const de::individual &vec)
    {
      auto trial(ind);
      trial.apply_decision_vector(dv_t(vec, dv.coords));

      return eva(trial);
    });

    search de_search(de_prob, de_eva);
    const auto res(de_search.run());

    ind.apply_decision_vector(dv_t(res.best_individual(), dv.coords));
    return res.best_measurements().fitness;
  }
};

/// Convenience API for DE-based numerical refinement.
template<Evaluator E>
std::optional<evaluator_fitness_t<E>> optimise(const numerical_optimiser &opt,
                                               evaluator_individual_t<E> &ind,
                                               const E &eva)
{
  return opt.optimise(ind, eva, refinement_backend {});
}

}  // namespace ultra::de

#endif  // include guard
