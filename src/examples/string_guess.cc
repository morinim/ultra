/*
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/string_guessing_tutorial
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

#include <iostream>
#include <string>

const std::string target = "Hello World";
const std::string CHARSET =
  " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!";

// The fitness function.
double fitness(const ultra::ga::individual &x)
{
  double found(0.0);

  for (std::size_t i(0); i < target.length(); ++i)
    if (target[i] == CHARSET[x[i]])
      ++found;

  return found;
}

int main()
{
  using namespace ultra;

  // A solution of this problem is a fixed length (`target.length()`) string of
  // characters in a given charset (`CHARSET`).
  ga::problem prob(target.length(), {0, CHARSET.size()});

  prob.params.population.individuals = 300;

  ga::search search(prob, fitness);
  auto result(search.run().best_individual());

  std::cout << "\nBest result: ";
  for (auto gene : result)
    std::cout << CHARSET[gene];

  std::cout << " (fitness " << fitness(result) << ")\n";
}
