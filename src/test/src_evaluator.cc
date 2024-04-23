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

#include <cstdlib>
#include <iostream>

#include "kernel/gp/src/evaluator.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/problem.h"
#include "kernel/gp/src/variable.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SRC::EVALUATOR")
{

TEST_CASE("Concepts")
{
  using namespace ultra::src;

  CHECK(DataSet<dataframe>);

  CHECK(ErrorFunction<mae_error_functor<ultra::gp::individual>, dataframe>);
  CHECK(ErrorFunction<rmae_error_functor<ultra::gp::individual>, dataframe>);
  CHECK(ErrorFunction<mse_error_functor<ultra::gp::individual>, dataframe>);
  CHECK(ErrorFunction<count_error_functor<ultra::gp::individual>, dataframe>);
}

TEST_CASE("Omniscent Oracle")
{
  using namespace ultra;

  std::istringstream sr(R"(
        95.2425,  2.81
      1554,       6
      2866.5485,  7.043
      4680,       8
     11110,      10
     18386.0340, 11.38
     22620,      12
     41370,      14
     54240,      15
    168420,      20
)");

  ultra::src::problem pr(sr);
  CHECK(!pr.data().empty());

  pr.params.init();
  pr.setup_symbols();

  const auto *x1(static_cast<const src::variable *>(pr.sset.decode("X1")));
  CHECK(x1);

  const function *f_ife(pr.insert<ultra::real::ife>());

  const src::reg_oracle delphi(gp::individual(
    {
      {f_ife, {x1, 15.000, 54240.0000, 168420.0}},
      {f_ife, {x1, 14.000, 41370.0000, 0_addr}},
      {f_ife, {x1, 12.000, 22620.0000, 1_addr}},
      {f_ife, {x1, 11.380, 18386.0340, 2_addr}},
      {f_ife, {x1, 10.000,  1110.0000, 3_addr}},
      {f_ife, {x1,  8.000,  4680.0000, 4_addr}},
      {f_ife, {x1,  7.043,  2866.5485, 5_addr}},
      {f_ife, {x1,  6.000,  1554.0000, 6_addr}},
      {f_ife, {x1,  2.810,    95.2425, 7_addr}}
    }));

  CHECK(std::get<D_DOUBLE>(delphi(pr.data().front()))
        == doctest::Approx(95.2425));
}

}  // SRC::EVALUATOR
