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

#if !defined(ULTRA_FITNESS_H)
#define      ULTRA_FITNESS_H

#include <cmath>
#include <numeric>

#include "utility/misc.h"

namespace ultra
{

///
/// This is **NOT THE RAW FITNESS**. Raw fitness is stated in the natural
/// terminology of the problem: the better value may be either smaller (as when
/// raw fitness is error) or larger (as when raw fitness is food eaten, benefit
/// achieved...).
///
/// We use a **STANDARDIZED FITNESS**: a greater numerical value is **always**
/// a better value (in many examples the optimal value is `0`, but this isn't
/// strictly necessary).
///
/// If, for a particular problem, a greater value of raw fitness is better,
/// standardized fitness equals the raw fitness for that problem (otherwise
/// standardized fitness must be computed from raw fitness).
///
/// \warning
/// The definition of standardized fitness given here is different from that
/// used in Koza's "Genetic Programming: On the Programming of Computers by
/// Means of Natural Selection". In the book a **LOWER** numerical value is
/// always a better one.
/// The main difference is that Vita attempts to maximize the fitness (while
/// other applications try to minimize it).
/// We chose this convention since it seemed more natural (a greater fitness
/// is a better fitness; achieving a better fitness means to maximize the
/// fitness). The downside is that sometimes we have to manage negative
/// numbers, but for our purposes it's not so bad.
/// Anyway maximization and minimization problems are basically the same: the
/// solution of `max(f(x))` is the same as `-min(-f(x))`. This is usually
/// all you have to remember when dealing with examples/problems expressed
/// in the other notation.
///
template<class F> concept Fitness = requires(F f1, F f2)
{
  requires std::totally_ordered<F>;

  {f1 + f2} -> std::convertible_to<F>;
  {f1 - f2} -> std::convertible_to<F>;
  {f1 * f2} -> std::convertible_to<F>;
  {f1 * double()} -> std::convertible_to<F>;
  {f1 / f2} -> std::convertible_to<F>;

  // This also requires that F is a signed type.
  requires F(-1) < F(0);
};

#include "kernel/fitness.tcc"

}  // namespace ultra

#endif  // include guard
