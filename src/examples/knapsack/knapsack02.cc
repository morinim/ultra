/*
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/knapsack_tutorial
 */

#include "kernel/ultra.h"

#include <iostream>
#include <string>

const std::vector file_sizes =
{
  1305892864, 1385113088, 856397968, 1106152425, 1647145093,
  1309917696, 1096825032, 1179242496, 1347631104, 696451130,
  746787826, 1080588288, 1165223499, 1181095818, 749898444, 1147613713,
  1280205208, 1242816512, 1189588992, 1232630196, 1291995024,
  911702020, 1678225920, 1252273456, 934001123, 863237392, 1358666176,
  1714134790, 1131848814, 1399329280, 1006665732, 1198348288,
  1090000441, 716904448, 677744640, 1067359748, 1646347388, 1266026326,
  1401106432, 1310275584, 1093615634, 1371899904, 736188416,
  1421438976, 1385125391, 1324463502, 1489042122, 1178813212,
  1239236096, 1258202316, 1364644352, 557194146, 555102962, 1383525888,
  710164700, 997808128, 1447622656, 1202085740, 694063104, 1753882504,
  1408100352
};

const auto target_size(8547993600);

// The fitness function.
double fitness(const ultra::de::individual &x)
{
  std::intmax_t sum(0);
  for (std::size_t i(0); i < x.parameters(); ++i)
    if (x[i] > 0.0 && sum + file_sizes[i] <= target_size)
      sum += file_sizes[i];

  return sum;
}

int main()
{
  using namespace ultra;

  // A solution of this problem is a fixed length (`file_sizes.size()`) vector
  // of boolean (file present / not present).
  de::problem prob(file_sizes.size(), {-1.0, 1.0});

  de::search search(prob, fitness);
  auto result(search.run(5).best_individual());

  std::cout << "\nBest result";
  for (std::size_t i(0); i < result.parameters(); ++i)
    if (result[i] > 0.0)
      std::cout << ' ' << file_sizes[i];

  std::cout << "\nFitness: " << static_cast<std::intmax_t>(fitness(result))
            << '\n';
}
