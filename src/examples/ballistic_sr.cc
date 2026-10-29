/*
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/ballistic_sr
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

enum dim : ultra::symbol::category_t
{
  length        = 0,  // Target output category (root instruction)
  dimensionless = 1,  // THETA, pure scalars, transcendental domain/range
  velocity      = 2,  // v0
  acceleration  = 3,  // g
  energy_spec   = 4   // v0^2 (L^2 T^-2)
};

int main()
{
  using namespace ultra;

  // TRAINING DATA: `Columns = { R (target), theta, v0, g }`.
  // Target `R` is column `0` (category `0: length`).
  const src::raw_data training =
  {
    {"R",     "THETA", "V0",   "G"},
    { 5.0968, 0.2618,  10.0,   9.81},  // 15 deg (2*theta = 30 deg)
    { 8.8280, 0.5236,  10.0,   9.81},  // 30 deg (2*theta = 60 deg)
    {10.1937, 0.7854,  10.0,   9.81},  // 45 deg (2*theta = 90 deg)
    { 8.8280, 1.0472,  10.0,   9.81},  // 60 deg
    {40.7747, 0.7854,  20.0,   9.81},  // 45 deg, v0=20
    {11.1895, 0.5236,  10.0,   7.74},  // Synthetic g
    {20.3874, 0.7854,  10.0,   4.905}  // Half-g
  };

  src::dataframe::params df_params;
  df_params.strong_data_typing();  // each column receives its own category

  src::problem prob(src::dataframe(training, df_params),
                    symbol_init::variables);

  // CONSTANTS.
  prob.insert<real::number>(-5, 5, dim::dimensionless);
  prob.insert<real::number>(-10, 10, dim::length);
  prob.insert<real::number>(-10, 10, dim::energy_spec);

  // Transcendental functions: strictly closed on category 1 (dimensionless).
  prob.insert<real::sin>(dim::dimensionless);
  prob.insert<real::cos>(dim::dimensionless);

  // Dimensionless arithmetic.
  prob.insert<real::mul>(dim::dimensionless,
                         {dim::dimensionless, dim::dimensionless});

  // Dimension-transforming arithmetic.
  // `v0 * v0 -> v0^2`
  prob.insert<real::mul>(dim::energy_spec, {dim::velocity, dim::velocity});

  // `(v0^2) / g -> length`
  prob.insert<real::div>(dim::length, {dim::energy_spec, dim::acceleration});

  // `length * dimensionless -> length`
  prob.insert<real::mul>(dim::length, {dim::length, dim::dimensionless});

  // SEARCH.
  prob.params.evolution.generations = 200;
  src::search s(prob);

  const auto result(s.run());

  std::cout << "Discovered Law:\n"
            << out::c_language << result.best_individual() << '\n';
}
