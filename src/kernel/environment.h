/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_ENVIRONMENT_H)
#define      ULTRA_ENVIRONMENT_H

#include "kernel/interval.h"
#include "kernel/symbol.h"

namespace ultra
{

namespace out
{
/// Rendering format used to print an individual.
enum print_format_t {list_f,  // default value
                     dump_f, graphviz_f, in_line_f, tree_f,
                     language_f,
                     c_language_f = language_f + symbol::c_format,
                     cpp_language_f = language_f + symbol::cpp_format,
                     python_language_f = language_f + symbol::python_format};
}  // namespace out

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
    /// `0` means undefined (auto-tune).
    std::size_t individuals {0};

    /// Number of layers the population is structured on.
    ///
    /// \warning
    /// When the evolution strategy is `basic_std_es`, setting `layers > 1` is
    /// like running multiple populations autonomously (there isn't any
    /// interaction among layers). A value greater than one is usually choosen
    /// for `basic_alps_es` or with other strategies that allow migrants.
    ///
    /// \note
    /// `0` means undefined (auto-tune).
    std::size_t layers {1};
  } population;

  struct evolution_parameters
  {
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
    /// A negative value means means undefined (auto-tune).
    double p_cross {-1.0};

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

  struct de_parameters
  {
    /// Weighting factor range (aka differential factor range).
    ///
    /// It has been found recently that selecting the weight from the
    /// interval `[0.5, 1.0]` randomly for each generation or for each
    /// difference vector, a technique called dither, improves convergence
    /// behaviour significantly, especially for noisy objective functions.
    interval_t<double> weight {0.5, 1.0};
  } de;

  environment &init();
  [[nodiscard]] bool is_valid(bool) const;
};  // environment

}  // namespace ultra

#endif  // include guard
