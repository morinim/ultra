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

#if !defined(ULTRA_ENVIRONMENT_H)
#define      ULTRA_ENVIRONMENT_H

#include "kernel/alps.h"
#include "kernel/interval.h"
#include "kernel/symbol.h"

namespace ultra
{

struct environment
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

    /// Initial number of layers the population is structured on.
    ///
    /// \warning
    /// Setting `init_layers > 1` with the standard evolution strategy is like
    /// running multiple populations autonomously (there isn't any direct
    /// interaction among layers; fitness values could be shared via cache).
    /// A value greater than one is required for ALPS or other strategies
    /// that allow migrants.
    ///
    /// \note
    /// `0` means auto-tune.
    std::size_t init_layers {1};

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
    /// - https://github.com/morinim/vita/wiki/bibliography#6
    /// - https://github.com/morinim/vita/wiki/bibliography#7
    /// - https://github.com/morinim/vita/wiki/bibliography#8
    ///
    /// \note
    /// - `0` means auto-tune;
    //  - `1` is the standard recombination (perform `1` crossover);
    //  - larger values enable the brood recombination method (more than one
    //    crossover).
    unsigned brood_recombination {0};

    /// An elitist algorithm is one that ALWAYS retains in the population the
    /// best individual found so far. With higher elitism the population will
    /// converge quicker but losing diversity.
    ///
    /// \note
    /// - `0.0` disable elitism
    /// - `1.0` always applies elitism
    /// - values outside the `[0.0;1.0]` range mean auto-tune
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

    /// Crossover probability.
    ///
    /// \note
    /// A negative value means means auto-tune.
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
    /// A negative value means auto-tune.
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
    interval_t<double> weight {0.5, 1.0};
  } de;

  environment &init();
  [[nodiscard]] bool is_valid(bool) const;
};  // environment

}  // namespace ultra

#endif  // include guard
