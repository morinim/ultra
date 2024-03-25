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

#include <algorithm>
#include <cstdlib>

#include "kernel/symbol_set.h"
#include "kernel/gp/primitive/real.h"

#include "utility/timer.h"
#include "utility/xoshiro256ss.h"

int main()
{
  using namespace ultra;

  real::abs ultra_abs;
  real::add ultra_add;
  real::add ultra_aq;
  real::add ultra_cos;
  real::add ultra_div;
  real::add ultra_ife;
  real::add ultra_ifl;
  real::add ultra_ifz;
  real::add ultra_ln;
  real::add ultra_max;
  real::add ultra_mod;
  real::add ultra_mul;
  real::add ultra_real;
  real::add ultra_sin;
  real::add ultra_sqrt;
  real::add ultra_sub;

  volatile std::size_t out(0);
  const unsigned sup(100000000);

  const std::vector<internal::w_symbol::weight_t> weights =
  {
    100, 200, 50, 50, 70, 50, 50, 50, 50, 50, 70, 100, 200, 50, 50, 200
  };

  std::discrete_distribution dd(weights.begin(), weights.end());

  const std::vector syms =
  {
    internal::w_symbol( &ultra_abs, weights[ 0]),
    internal::w_symbol( &ultra_add, weights[ 1]),
    internal::w_symbol(  &ultra_aq, weights[ 2]),
    internal::w_symbol( &ultra_cos, weights[ 3]),
    internal::w_symbol( &ultra_div, weights[ 4]),
    internal::w_symbol( &ultra_ife, weights[ 5]),
    internal::w_symbol( &ultra_ifl, weights[ 6]),
    internal::w_symbol( &ultra_ifz, weights[ 7]),
    internal::w_symbol(  &ultra_ln, weights[ 8]),
    internal::w_symbol( &ultra_max, weights[ 9]),
    internal::w_symbol( &ultra_mod, weights[10]),
    internal::w_symbol( &ultra_mul, weights[11]),
    internal::w_symbol(&ultra_real, weights[12]),
    internal::w_symbol( &ultra_sin, weights[13]),
    internal::w_symbol(&ultra_sqrt, weights[14]),
    internal::w_symbol( &ultra_sub, weights[15])
  };

  // -------------------------------------------------------------------------

  // Standard roulette algorithm.
  // This is simple and fast.
  const unsigned sum(std::accumulate(syms.begin(), syms.end(), 0,
                                     [](auto s, const auto &v)
                                     {
                                       return s + v.weight;
                                     }));

  ultra::timer t;

  const auto slot(random::sup(sum));

  for (unsigned i(0); i < sup; ++i)
  {
    std::size_t j(0);
    for (auto wedge(syms[j].weight); wedge <= slot; wedge += syms[++j].weight)
      out = j;
  }

  std::cout << "Std roulette  - Elapsed: " << t.elapsed().count() << "ms\n";

  // -------------------------------------------------------------------------

  // Roulette-wheel selection via stochastic acceptance (Adam Lipowski,
  // Dorota Lipowska).
  const unsigned max(std::ranges::max(syms, [](const auto &a, const auto &b)
                                            {
                                              return a.weight < b.weight;
                                            }).weight);

  t.restart();

  for (unsigned i(0); i < sup; ++i)
  {
    for (;;)
    {
      const auto &s(random::element(syms));

      if (random::sup(max) < s.weight)
      {
        out = s.weight;
        break;
      }
    }
  }

  std::cout << "Stochastic    - Elapsed: " << t.elapsed().count() << "ms\n";

  // -------------------------------------------------------------------------

  // Roulette wheel with unknown sum of the weights.
  // See https://eli.thegreenplace.net/
  // The interesting property of this algorithm is that you don't need to know
  // the sum of weights in advance in order to use it. The method is cool, but
  // slower than the standard roulette.
  t.restart();

  for (unsigned i(0); i < sup; ++i)
  {
    unsigned total(0);
    std::size_t winner(0);

    for (std::size_t j(0); j < syms.size(); ++j)
    {
      total += syms[j].weight;
      if (random::sup(total + 1) < syms[j].weight)
        winner = j;
    }

    out = winner;
  }

  std::cout << "Unknown sum   - Elapsed: " << t.elapsed().count() << "ms\n";

  // -------------------------------------------------------------------------

  t.restart();

  // std::discrete_distribution
  for (unsigned i(0); i < sup; ++i)
  {
    out = dd(random::engine());
  }

  std::cout << "Discrete dist - Elapsed: " << t.elapsed().count() << "ms\n";

  // -------------------------------------------------------------------------

  // Often the fastest way to produce a realization of a random variable `X` in
  // a computer is to create a big table where each outcome `i` is inserted a
  // number of times proportional to `P(X=i)`.

  std::vector<internal::w_symbol> big_syms;
  for (const auto &ws : syms)
    for (unsigned i(0); i < ws.weight; ++i)
      big_syms.push_back(ws);

  t.restart();

  for (unsigned i(0); i < sup; ++i)
  {
    out = random::sup(big_syms.size());
  }

  std::cout << "Big table     - Elapsed: " << t.elapsed().count() << "ms\n";

  return !out;  // just to stop some warnings
}
