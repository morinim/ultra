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

#if !defined(ULTRA_PARAMETERS_H)
#define      ULTRA_PARAMETERS_H

#include "kernel/alps.h"
#include "kernel/interval.h"
#include "kernel/symbol.h"

#include <filesystem>

namespace ultra
{

struct parameters
{
  struct slp_parameters
  {
    /// The number of genes (maximum length of an evolved program in the
    /// population).
    ///
    /// Code length have to be chosen before population is created and cannot
    /// be changed afterwards.
    ///
    /// \note
    /// A length of `0` means undefined (auto-tune).
    std::size_t code_length {0};
  } slp;

  struct population_parameters
  {
    /// Number of individuals in a **layer** of the population.
    ///
    /// \note
    /// `0` means auto-tune.
    std::size_t individuals {0};

    /// Initial number of subgroups the population is structured on.
    ///
    /// \warning
    /// Setting `init_subgroups > 1` with the standard evolution strategy is
    /// like running multiple concurrent populations autonomously (there isn't
    /// any direct interaction among subgroups; fitness values could be shared
    /// via cache).
    /// For ALPS or other strategies that allow migrants the situation is more
    /// complex.
    ///
    /// \note
    /// `0` means auto-tune.
    std::size_t init_subgroups {0};

    /// Minimum number of individuals in a layer of the population.
    ///
    /// Some evolution strategies dynamically change the population size. This
    /// parameter avoids to drop below a predefined limit
    ///
    /// \note
    /// `0` means auto-tune.
    std::size_t min_individuals {0};
  } population;

  struct evolution_parameters
  {
    /// This parameter controls the brood recombination/selection level (`1` to
    /// turn it off).
    ///
    /// In nature it's common for organisms to produce many offspring and then
    /// neglect, abort, resorb, eat some of them or allow them to eat each
    /// other. There are various reasons for this behavior (e.g. progeny choice
    /// hypothesis). The phenomenon is known variously as soft selection, brood
    /// selection, spontaneous abortion. The "bottom line" of this behaviour in
    /// nature is the reduction of parental resource investment in offspring who
    /// are potentially less fit than others.
    ///
    /// \see
    /// - https://github.com/morinim/ultra/wiki/bibliography#6
    /// - https://github.com/morinim/ultra/wiki/bibliography#7
    ///
    /// \note
    /// - `0` means auto-tune;
    //  - `1` is the standard recombination (perform `1` crossover);
    //  - larger values enable the brood recombination method (more than one
    //    crossover).
    unsigned brood_recombination {0};

    /// Controls how strongly the search tries to preserve the best
    /// individuals.
    /// Higher elitism increases selection pressure and reduces the chance that
    /// exceptional individuals are lost, at the cost of diversity.
    ///
    /// \note
    /// - `0.0` disables elitist protection;
    /// - `1.0` applies the maximum configured protection;
    /// - values outside the `[0.0; 1.0]` range mean auto-tune.
    double elitism {-1.0};

    /// Maximun number of generations allowed before terminate a run.
    ///
    /// \note
    /// `0` means auto-tune.
    unsigned generations {0};

    /// This is used for the trivial geography scheme.
    /// The population is viewed as having a 1-dimensional spatial structure -
    /// actually a circle, as we consider the first and last locations to be
    /// adiacent. The production of an individual from location `i` is permitted
    /// to involve only parents from `i`'s local neightborhood, where the
    /// neightborhood is defined as all individuals within distance
    /// `mate_zone` of `i`.
    ///
    /// \note
    /// - `0` means auto-tune.
    /// - `std::numeric_limits<std::size_t>::max()` (or a large enough number)
    ///   disables the scheme.
    ///
    /// \see https://github.com/morinim/ultra/wiki/bibliography#3
    std::size_t mate_zone {0};

    /// Used by some evolution strategies to stop evolution when there aren't
    /// improvements within the given number of generations.
    ///
    /// `0` means auto-tune.
    unsigned max_stuck_gen = {0};

    /// Crossover probability.
    ///
    /// \note
    /// Values outside the `[0.0;1.0]` range mean auto-tune.
    double p_cross {-1.0};

    /// Mutation rate probability.
    ///
    /// Mutation is one of the principal "search operators" used to transform
    /// individuals in evolutionary algorithms. It causes random changes in
    /// the genes.
    ///
    /// \warning
    /// `p_cross + p_mutation != 1.0`: `p_mutation` is the probability to
    /// mutate a gene; it's not the probability of choosing the mutation
    /// operator (which depends depends on the recombination algorithm).
    ///
    /// \note
    /// Values outside the `[0.0;1.0]` range mean auto-tune.
    double p_mutation {-1.0};

    /// Size of the tournament to choose the parents from.
    ///
    /// Tournament sizes tend to be small relative to the population size. The
    /// ratio of tournament size to population size can be used as a measure of
    /// selective pressure.
    ///
    /// \note
    /// - A tournament size of `1` would be equivalent to selecting individuals
    ///   at random.
    /// - A length of `0` means auto-tune.
    std::size_t tournament_size {0};

    /// Maximum allowed value for `evolution.tournament_size`.
    ///
    /// This guard prevents pathological configurations that would make
    /// tournament selection excessively expensive.
    static constexpr std::size_t max_tournament_size {10000};
  } evolution;

  alps::parameters alps;

  struct de_parameters
  {
    /// Weighting factor range (aka differential factor range).
    ///
    /// It has been found recently that selecting the weight from the
    /// interval `[0.5, 1.0]` randomly for each generation or for each
    /// difference vector, a technique called dither, improves convergence
    /// behaviour significantly, especially for noisy objective functions.
    ///
    /// \see https://github.com/morinim/ultra/wiki/bibliography#5
    interval<double> weight {0.5, 1.0};
  } de;

  /// Parameters controlling numerical optimisation of GP individuals.
  struct refinement_parameters
  {
    struct de_numerical_refinement_parameters
    {
      /// Relative radius of the search interval.
      ///
      /// For a parameter with current value `v`, the search interval is:
      ///   `[v - delta, v + delta]`
      /// where:
      ///   `delta = max(|v| * rel_radius, min_radius)`
      ///
      /// This makes the search scale with the magnitude of the parameter.
      double rel_radius {0.25};

      /// Minimum radius of the search interval.
      ///
      /// Ensures a non-degenerate search space even when the current value is
      /// close to zero.
      double min_radius {1.0};

      /// Population size used by the DE optimiser.
      ///
      /// Controls the number of candidate solutions evolved at each iteration.
      std::size_t individuals {20};

      /// Number of generations of the DE optimiser.
      ///
      /// Higher values increase optimisation quality at the cost of runtime.
      std::size_t generations {20};
    } de;

    /// Fraction of individuals to be refined.
    ///
    /// Controls how many individuals are selected (per generation) for local
    /// refinement.
    ///
    /// The value is interpreted as a fraction in the range `[0, 1]`:
    /// - `0.0` disables refinement;
    /// - `1.0` applies refinement to the entire population;
    /// - intermediate values select a proportion of individuals (typically
    ///   via random sampling).
    double fraction {0.01};
  } refinement;

  struct cache_parameters
  {
    /// `2^size` is the number of elements of the cache. `0` disables caching.
    unsigned size {16};

    /// Filename used for persistance of the cache. An empty name is used to
    /// skip serialization.
    std::filesystem::path serialization_file {};
  } cache;

  struct team_parameters
  {
    /// Default team size:
    /// - `0` means auto tune;
    /// - `1` is used for debugging.
    std::size_t individuals {3};
  } team;

  parameters &init();
  [[nodiscard]] bool needs_init() const noexcept;

  [[nodiscard]] bool is_valid(bool) const;
};  // parameters

}  // namespace ultra

#endif  // include guard
