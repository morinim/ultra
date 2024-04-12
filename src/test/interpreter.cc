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
#include <numbers>

#include "kernel/gp/interpreter.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("INTERPRETER")
{

TEST_CASE_FIXTURE(fixture1, "Run")
{
  using namespace ultra;

  SUBCASE("Abs")
  {
    gp::individual i(
      {
        {f_abs, {x->instance()}}    // [0] ABS $X
      });

    const auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(x_val));
  }

  SUBCASE("Add")
  {
    gp::individual i(
      {
        {f_add, {x->instance(), y->instance()}}    // [0] ABS $X, $Y
      });

    const auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(x_val + y_val));
  }

  SUBCASE("Aq")
  {
    gp::individual i(
      {
        {f_aq, {z, c1->instance()}}    // [0] ABS Z(), $C1
      });

    for (unsigned j(0); j < 100; ++j)
    {
      auto &rz(static_cast<Z *>(z)->val);

      rz = random::between(-1000000.0, 1000000.0);
      const auto ret(run(i));

      CHECK(real::base(ret) == doctest::Approx(rz / std::numbers::sqrt2));
    }
  }

  SUBCASE("Div")
  {
    gp::individual i(
      {
        {f_div, {z, 1.0}}    // [0] DIV Z(), $1.0
      });

    for (unsigned j(0); j < 100; ++j)
    {
      auto &rz(static_cast<Z *>(z)->val);

      rz = random::between(-1000000.0, 1000000.0);
      const auto ret(run(i));

      CHECK(real::base(ret) == doctest::Approx(rz));
    }
  }

  SUBCASE("Idiv")
  {
    gp::individual i(
      {
        {f_idiv, {x->instance(), 0.0}}    // [0] IDIV $X, $0.0
      });

    const auto ret(run(i));
    CHECK(std::isinf(real::base(ret)));
  }

  SUBCASE("Ifz")
  {
    gp::individual i(
      {
        {f_sub, {z, z}},           // [0] SUB Z(), Z()
        {f_ifz, {z, z, 0_addr}}    // [0] IFZ Z(), Z(), [0]
      });

    for (unsigned j(0); j < 100; ++j)
    {
      auto &rz(static_cast<Z *>(z)->val);

      rz = random::between(-1000000.0, 1000000.0);
      const auto ret(run(i));

      CHECK(real::base(ret) == doctest::Approx(0.0));
    }
  }

  SUBCASE("Sqrt")
  {
    gp::individual i(
      {
        {f_sqrt, {-1.0}}    // [0] SQRT $-1.0
      });

    const auto ret(run(i));
    CHECK(!has_value(ret));
  }

  SUBCASE("Mix 1")
  {
    gp::individual i(
      {
        {f_add, {3.0, 2.0}},       // [0] ADD $3.0, $2.0
        {f_add, {0_addr, 1.0}},    // [1] ADD [0], $1.0
        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
      });

    auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(1.0));
  }

  SUBCASE("Mix 2")
  {
    gp::individual i(
      {
        {f_mul, {z, 2.0}},         // [0] MUL Z(), $2.0
        {f_add, {z, z}},           // [1] ADD Z(), Z()
        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
      });

    for (unsigned j(0); j < 100; ++j)
    {
      auto &rz(static_cast<Z *>(z)->val);

      rz = random::between(-1000000.0, 1000000.0);
      const auto ret(run(i));

      CHECK(real::base(ret) == doctest::Approx(0.0));
    }
  }
}

}  // TEST_SUITE("INTERPRETER")
