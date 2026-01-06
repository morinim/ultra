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
 *  \see
 *  https://github.com/morinim/ultra/wiki/scheduling_tutorial
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include <chrono>
#include <vector>

#include "kernel/ultra.h"

// Examples taken from "Differential Evolution in Discrete Optimization" by
// Daniel Lichtblau.
// See https://github.com/morinim/vita/wiki/bibliography#8

//`n_machines` homogeneous machines (i.e. each job time is independent of the
// machine used.
const int n_machines = 5;

// `n_jobs` with random durations.
const int n_jobs = 50;
std::vector<std::chrono::hours> job_duration(n_jobs);

// Assuming a total time period of one day.
double f(ultra::de::individual start)
{
  double ret(0.0);

  start.apply_each([](auto &v) { v = std::round(v); });

  for (unsigned i(0); i < start.parameters(); ++i)
  {
    // A job starts at a nonnegative time.
    if (start[i] < 0.0)
      ret += start[i];

    // A job starts more than its length prior to the 24 hour limit.
    const auto end(start[i] + job_duration[i].count());
    if (end >= 24.0)
      ret -= end - 24.0;

    int occupied(1);
    for (unsigned j(0); j < start.parameters(); ++j)
      if (j != i
          && start[j] <= start[i]
          && start[j] + job_duration[j].count() > start[i])
        ++occupied;

    if (occupied > n_machines)
      ret -= occupied - n_machines;
  }

  return ret;
}

int main()
{
  using namespace ultra;
  using namespace std::chrono_literals;

  log::reporting_level = log::lINFO;

  std::ranges::generate(job_duration,
                        []
                        {
                          return std::chrono::hours(random::between(1, 4));
                        });

  std::cout << "Total time required: "
            << std::accumulate(job_duration.begin(), job_duration.end(),
                               0h).count()
            << '\n';

  // A candidate solution is a sequence of `n_jobs` doubles in the
  // `[-0.5, 23.5[` interval.
  de::problem prob(n_jobs, {-0.5, 23.5});

  prob.params.population.individuals =   50;
  prob.params.evolution.generations  = 2000;

  de::search search(prob, f);

  const auto res(search.run().best_individual);

  for (unsigned i(0); i < n_jobs; ++i)
    std::cout << i << ' ' << std::round(res[i]) << ' '
              << job_duration[i].count() << '\n';

  std::cout << "Fitness: " << f(res) << std::endl;

  // A simple script for GnuPlot:
  // set xtics 1
  // set ytics 2
  // set grid xtics ytics
  // plot [x=0:24][y=-0.5:50.5] "test.dat" using 2:1:3:(0)
  //      w vectors head filled lw 2 notitle
}
