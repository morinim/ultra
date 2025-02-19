/*
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/8queens_tutorial
 */

#include <iostream>

#include "kernel/ultra.h"

const int NQUEENS(8);

int main()
{
  using namespace ultra;

  // A candidate solution is a sequence of `NQUEENS` integers in the
  // `[0, NQUEENS[` interval.
  // For instance `{4, 2, 0, 6, 1, 7, 5, 3}` means first queen on
  // `a5`, second queen on `b3`, third queen on `c1`, fourth queen on
  // `d7`...
  ga::problem prob(NQUEENS, {0, NQUEENS});

  // Fitness function.
  auto f = [](const ga::individual &x)
  {
    double attacks(0);

    for (int queen(0); queen < NQUEENS - 1; ++queen)  // skips the last queen
    {
      const int row(x[queen]);

      for (int i(queen + 1); i < NQUEENS; ++i)
      {
        const int other_row(x[i]);

        if (other_row == row                            // same row
            || std::abs(other_row - row) == i - queen)  // or diagonal
          ++attacks;
      }
    }

    return -attacks;
  };

  // Let's go.
  ga::search search(prob, f);
  auto result = search.run();

  // Prints result.
  std::cout << "\nBest result: [";
  for (auto gene : result.best_individual)
    std::cout << " " << gene;

  std::cout << " ]   (fitness " << *result.best_measurements.fitness << ")\n";
}
