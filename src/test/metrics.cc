/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/gp/src/metrics.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/src/oracle.h"
#include "kernel/gp/src/variable.h"

#include "test/fixture1.h"

#include <sstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("Metrics")
{

TEST_CASE("mae accepts plain predictor")
{
  using namespace ultra;

  struct predictor
  {
    value_t operator()(const std::vector<value_t> &x) const
    {
      return std::get<D_DOUBLE>(x[0]) + 1.0;
    }
  };

  std::istringstream test(R"(
    2.0, 1.0
    3.0, 2.0
    4.0, 3.0
  )");

  const src::dataframe df(test);

  CHECK(src::metrics::mae(predictor{}, df) == doctest::Approx(0.0));
}

TEST_CASE_FIXTURE(fixture1, "mae")
{
  using namespace ultra;

  SUBCASE("regression base")
  {
    src::variable x0(0, "X0");
    src::variable x1(1, "X1");

    // `2*(X0 + X1) + 1.0`
    gp::individual i({
                       {f_add, {&x0, &x1}},
                       {f_add, {0_addr, 1.0}},
                       {f_add, {1_addr, 0_addr}}
                     });

    src::reg_oracle oracle(i);

    {
      std::istringstream test1(R"(
         1.0, 0.0, 0.0
         3.0, 1.0, 0.0
         5.0, 1.0, 1.0
         7.0, 2.0, 1.0
         9.0, 2.0, 2.0
      )");

      CHECK(src::metrics::mae(oracle, src::dataframe(test1))
            == doctest::Approx(0.0));
    }

    {
      std::istringstream test2(R"(
         1.0, 0.0, 0.0
         0.0, 1.0, 0.0
         5.0, 1.0, 1.0
         0.0, 2.0, 1.0
         9.0, 2.0, 2.0
         0.0, 3.0, 2.0
      )");

      CHECK(src::metrics::mae(oracle, src::dataframe(test2))
            == doctest::Approx(3.5));
    }

    {
      std::istringstream test3(R"(
         0.0, 0.0, 0.0
         0.0, 1.0, 0.0
         0.0, 1.0, 1.0
         0.0, 2.0, 1.0
         0.0, 2.0, 2.0
         0.0, 3.0, 2.0
      )");

      CHECK(src::metrics::mae(oracle, src::dataframe(test3))
            == doctest::Approx(6.0));
    }
  }

  SUBCASE("regression exact polynomial")
  {
    src::variable x0(0, "X0");

    // `(X0/X0 + X0) * (X0 + X0*X0*X0)` = `X0 + X0^2 + X0^3 + X0^4`
    gp::individual i({
                       {f_mul, {&x0, &x0}},
                       {f_mul, {0_addr, &x0}},
                       {f_add, {&x0, 1_addr}},
                       {f_div, {&x0, &x0}},
                       {f_add, {3_addr, &x0}},
                       {f_mul, {4_addr, 2_addr}}
                     });

    src::reg_oracle oracle(i);

    std::istringstream test(R"(
          95.2425,   2.81
        1554.0,      6.0
        2866.5485,   7.043
        4680.0,      8.0
       11110.0,     10.0
       18386.0340,  11.38
       22620.0,     12.0
       41370.0,     14.0
       54240.0,     15.0
      168420.0,     20.0)");

    CHECK(src::metrics::mae(oracle, src::dataframe(test)) < 1e-4);
  }
}

TEST_CASE("accuracy accepts plain classification predictor")
{
  using namespace ultra;

  struct predictor
  {
    src::classification_result tag(const std::vector<value_t> &x) const
    {
      return {std::get<D_DOUBLE>(x[0]) > 0.0 ? 1uz : 0uz, 1.0};
    }
  };

  std::istringstream test(R"(
    "N", -1.0
    "P",  2.0
    "N", -3.0
    "P",  4.0
  )");

  const src::dataframe df(test);

  CHECK(src::metrics::accuracy(predictor{}, df) == doctest::Approx(1.0));
}

TEST_CASE_FIXTURE(fixture1, "classification accuracy")
{
  using namespace ultra;

  src::variable x1(0, "X1");
  src::variable x2(0, "X2");
  src::variable x3(0, "X3");
  src::variable x4(0, "X4");

  gp::individual i({
      {f_mul, {&x4, &x3}},     // [0] FMUL X4 X3
      {f_sub, {&x1, 0_addr}},  // [1] FMUL X1 [0]
      {f_add, {&x2, 1_addr}}   // [2] FADD X2 [1]
    });

  std::istringstream test(R"(
      "S", 5.1, 3.5, 1.4, 0.2
      "S", 4.9, 3.0, 1.4, 0.2
      "S", 4.7, 3.2, 1.3, 0.2
      "S", 4.6, 3.1, 1.5, 0.2
      "E", 7.0, 3.2, 4.7, 1.4
      "E", 6.4, 3.2, 4.5, 1.5
      "E", 6.9, 3.1, 4.9, 1.5
      "E", 5.5, 2.3, 4.0, 1.3
      "I", 6.3, 3.3, 6.0, 2.5
      "I", 5.8, 2.7, 5.1, 1.9
      "I", 7.1, 3.0, 5.9, 2.1
      "I", 6.3, 2.9, 5.6, 1.8)");

  const src::dataframe df(test);

  src::gaussian_oracle oracle(i, df);

  CHECK(src::metrics::accuracy(oracle, df) == doctest::Approx(0.75));
}

TEST_CASE("mae ignores missing predictions")
{
  using namespace ultra;

  struct predictor
  {
    value_t operator()(const std::vector<value_t> &x) const
    {
      const auto v(std::get<D_DOUBLE>(x[0]));

      if (v < 0.0)
        return {};

      return v;
    }
  };

  std::istringstream test(R"(
    1.0,  1.0
    2.0,  2.0
    9.0, -1.0
  )");

  const src::dataframe df(test);

  CHECK(src::metrics::mae(predictor{}, df) == doctest::Approx(0.0));
}

TEST_CASE("mae returns infinity when all predictions are missing")
{
  using namespace ultra;

  struct predictor
  {
    value_t operator()(const std::vector<value_t> &) const { return {}; }
  };

  std::istringstream test(R"(
    1.0, 1.0
    2.0, 2.0
  )");

  const src::dataframe df(test);

  CHECK(std::isinf(src::metrics::mae(predictor{}, df)));
}

}  // TEST_SUITE
