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
  using namespace ultra;

  CHECK(src::DataSet<src::dataframe>);

  CHECK(src::ErrorFunction<src::mae_error_functor<gp::individual>,
                           src::dataframe>);
  CHECK(src::ErrorFunction<src::rmae_error_functor<gp::individual>,
                           src::dataframe>);
  CHECK(src::ErrorFunction<src::mse_error_functor<gp::individual>,
                           src::dataframe>);
  CHECK(src::ErrorFunction<src::count_error_functor<gp::individual>,
                           src::dataframe>);

  CHECK(Evaluator<src::mae_evaluator<gp::individual>>);
  CHECK(Evaluator<src::mae_evaluator<team<gp::individual>>>);
  CHECK(Evaluator<src::rmae_evaluator<gp::individual>>);
  CHECK(Evaluator<src::rmae_evaluator<team<gp::individual>>>);
  CHECK(Evaluator<src::mse_evaluator<gp::individual>>);
  CHECK(Evaluator<src::mse_evaluator<team<gp::individual>>>);
  CHECK(Evaluator<src::count_evaluator<gp::individual>>);
  CHECK(Evaluator<src::count_evaluator<team<gp::individual>>>);
}

TEST_CASE("Evaluators")
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

  const std::vector out = {95.2425, 1554.0, 2866.5485, 4680.0, 11110.0,
                           18386.0340, 22620.0, 41370.0, 54240.0, 168420.0};

  const gp::individual delphi(
    {
      {f_ife, {x1, 15.000, out[8], out[9]}},
      {f_ife, {x1, 14.000, out[7], 0_addr}},
      {f_ife, {x1, 12.000, out[6], 1_addr}},
      {f_ife, {x1, 11.380, out[5], 2_addr}},
      {f_ife, {x1, 10.000, out[4], 3_addr}},
      {f_ife, {x1,  8.000, out[3], 4_addr}},
      {f_ife, {x1,  7.043, out[2], 5_addr}},
      {f_ife, {x1,  6.000, out[1], 6_addr}},
      {f_ife, {x1,  2.810, out[0], 7_addr}}
    });

  const gp::individual delta1(
    {
      {f_ife, {x1, 15.000, out[8] + 1.0, out[9] + 1.0}},
      {f_ife, {x1, 14.000, out[7] + 1.0, 0_addr}},
      {f_ife, {x1, 12.000, out[6] + 1.0, 1_addr}},
      {f_ife, {x1, 11.380, out[5] + 1.0, 2_addr}},
      {f_ife, {x1, 10.000, out[4] + 1.0, 3_addr}},
      {f_ife, {x1,  8.000, out[3] + 1.0, 4_addr}},
      {f_ife, {x1,  7.043, out[2] + 1.0, 5_addr}},
      {f_ife, {x1,  6.000, out[1] + 1.0, 6_addr}},
      {f_ife, {x1,  2.810, out[0] + 1.0, 7_addr}}
    });

  const gp::individual delta2(
    {
      {f_ife, {x1, 15.000, out[8] + 2.0, out[9] + 2.0}},
      {f_ife, {x1, 14.000, out[7] + 2.0, 0_addr}},
      {f_ife, {x1, 12.000, out[6] + 2.0, 1_addr}},
      {f_ife, {x1, 11.380, out[5] + 2.0, 2_addr}},
      {f_ife, {x1, 10.000, out[4] + 2.0, 3_addr}},
      {f_ife, {x1,  8.000, out[3] + 2.0, 4_addr}},
      {f_ife, {x1,  7.043, out[2] + 2.0, 5_addr}},
      {f_ife, {x1,  6.000, out[1] + 2.0, 6_addr}},
      {f_ife, {x1,  2.810, out[0] + 2.0, 7_addr}}
    });

  const gp::individual huge1(
    {
      {f_ife, {x1, 15.000, out[8], out[9]}},
      {f_ife, {x1, 14.000, out[7], 0_addr}},
      {f_ife, {x1, 12.000, out[6], 1_addr}},
      {f_ife, {x1, 11.380, out[5], 2_addr}},
      {f_ife, {x1, 10.000, out[4], 3_addr}},
      {f_ife, {x1,  8.000, out[3], 4_addr}},
      {f_ife, {x1,  7.043, out[2], 5_addr}},
      {f_ife, {x1,  6.000, out[1], 6_addr}},
      {f_ife, {x1,  2.810, HUGE_VAL, 7_addr}}
    });

  const gp::individual huge2(
    {
      {f_ife, {x1, 15.000, out[8], out[9]}},
      {f_ife, {x1, 14.000, out[7], 0_addr}},
      {f_ife, {x1, 12.000, out[6], 1_addr}},
      {f_ife, {x1, 11.380, out[5], 2_addr}},
      {f_ife, {x1, 10.000, out[4], 3_addr}},
      {f_ife, {x1,  8.000, out[3], 4_addr}},
      {f_ife, {x1,  7.043, out[2], 5_addr}},
      {f_ife, {x1,  6.000, -HUGE_VAL, 6_addr}},
      {f_ife, {x1,  2.810, HUGE_VAL, 7_addr}}
    });

  SUBCASE("Delphi knows everything")
  {
    {
      const src::reg_oracle oracle(delphi);
      for (std::size_t i(0); const auto &e : pr.data())
        CHECK(std::get<D_DOUBLE>(oracle(e.input))
              == doctest::Approx(out[i++]));
    }

    {
      const src::reg_oracle oracle(delta1);
      for (std::size_t i(0); const auto &e : pr.data())
        CHECK(std::get<D_DOUBLE>(oracle(e.input))
              == doctest::Approx(out[i++] + 1));
    }

    {
      const src::reg_oracle oracle(delta2);
      for (std::size_t i(0); const auto &e : pr.data())
        CHECK(std::get<D_DOUBLE>(oracle(e.input))
              == doctest::Approx(out[i++] + 2));
    }
  }

  SUBCASE("MAE Evaluator")
  {
    src::mae_evaluator<gp::individual> mae(pr.data());
    CHECK(mae(delphi) == doctest::Approx(0.0));
    CHECK(mae(delta1) == doctest::Approx(-1.0));
    CHECK(mae(delta2) == doctest::Approx(-2.0));
    CHECK(std::isnan(mae(huge1)));
    CHECK(std::isnan(mae(huge2)));
  }

  SUBCASE("RMAE Evaluator")
  {
    src::rmae_evaluator<gp::individual> rmae(pr.data());
    CHECK(rmae(delphi) == doctest::Approx(0.0));
    CHECK(rmae(delta1) == doctest::Approx(-0.118876));
    CHECK(rmae(delta2) == doctest::Approx(-0.23666));
    CHECK(std::isnan(rmae(huge1)));
    CHECK(std::isnan(rmae(huge2)));
  }

  SUBCASE("MSE Evaluator")
  {
    src::mse_evaluator<gp::individual> mse(pr.data());
    CHECK(mse(delphi) == doctest::Approx(0.0));
    CHECK(mse(delta1) == doctest::Approx(-1.0));
    CHECK(mse(delta2) == doctest::Approx(-4.0));
    CHECK(std::isnan(mse(huge1)));
    CHECK(std::isnan(mse(huge2)));
  }

  SUBCASE("Count Evaluator")
  {
    src::count_evaluator<gp::individual> count(pr.data());
    CHECK(count(delphi) == doctest::Approx(0.0));
    CHECK(count(delta1) == doctest::Approx(-1.0));
    CHECK(count(delta2) == doctest::Approx(-1.0));
    CHECK(count(huge1) == doctest::Approx(-1.0 / pr.data().size()));
    CHECK(count(huge2) == doctest::Approx(-2.0 / pr.data().size()));
  }
}

}  // SRC::EVALUATOR
