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

#include "kernel/gp/interpreter.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <array>
#include <numbers>

TEST_SUITE("INTERPRETER")
{

TEST_CASE_FIXTURE(fixture1, "Run")
{
  using namespace ultra;

  auto &rz(static_cast<Z *>(z)->val);

  const auto repeat_with_various_z([&](auto check)
  {
    constexpr std::array samples =
    {
      -1000000.0, -1.0, 0.0, 1.0, 1000000.0
    };

    for (auto v : samples)
    {
      rz = v;
      check();
    }
  });

  SUBCASE("Abs")
  {
    const gp::individual i(
      {
        {f_abs, {x->instance()}}    // [0] ABS $X
      });

    const auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(x_val));
  }

  SUBCASE("Add")
  {
    const gp::individual i(
      {
        {f_add, {x->instance(), y->instance()}}    // [0] ADD $X, $Y
      });

    const auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(x_val + y_val));
  }

  SUBCASE("Aq")
  {
    const gp::individual i(
      {
        {f_aq, {z, c1->instance()}}    // [0] AQ Z(), $C1
      });

    repeat_with_various_z([&]
    {
      const auto ret(run(i));
      CHECK(real::base(ret) == doctest::Approx(rz / std::numbers::sqrt2));
    });
  }

  SUBCASE("Div")
  {
    const gp::individual i(
      {
        {f_div, {z, 1.0}}    // [0] DIV Z(), $1.0
      });

    repeat_with_various_z([&]
    {
      const auto ret(run(i));
      CHECK(real::base(ret) == doctest::Approx(rz));
    });
  }

  SUBCASE("Idiv")
  {
    const gp::individual i(
      {
        {f_idiv, {x->instance(), 0.0}}    // [0] IDIV $X, $0.0
      });

    const auto ret(run(i));
    CHECK(!has_value(ret));
  }

  SUBCASE("Ifz")
  {
    const gp::individual i(
      {
        {f_sub, {z, z}},           // [0] SUB Z(), Z()
        {f_ifz, {z, z, 0_addr}}    // [1] IFZ Z(), Z(), [0]
      });

    repeat_with_various_z([&]
    {
      const auto ret(run(i));
      CHECK(real::base(ret) == doctest::Approx(0.0));
    });
  }

  SUBCASE("Sqrt")
  {
    const gp::individual i(
      {
        {f_sqrt, {-1.0}}    // [0] SQRT $-1.0
      });

    const auto ret(run(i));
    CHECK(!has_value(ret));
  }

  SUBCASE("Mix 1")
  {
    const gp::individual i(
      {
        {f_add, {3.0, 2.0}},       // [0] ADD $3.0, $2.0
        {f_add, {0_addr, 1.0}},    // [1] ADD [0], $1.0
        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
      });

    const auto ret(run(i));
    CHECK(real::base(ret) == doctest::Approx(1.0));
  }

  SUBCASE("Mix 2")
  {
    const gp::individual i(
      {
        {f_mul, {z, 2.0}},         // [0] MUL Z(), $2.0
        {f_add, {z, z}},           // [1] ADD Z(), Z()
        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
      });

    repeat_with_various_z([&]
    {
      const auto ret(run(i));
      CHECK(real::base(ret) == doctest::Approx(0.0));
    });
  }
}

TEST_CASE_FIXTURE(fixture1, "Interpreter cache")
{
  using namespace ultra;

  auto &rz(static_cast<Z *>(z)->val);

  const gp::individual i(
    {
      {f_add, {z, z}},           // [0] ADD Z(), Z()
      {f_add, {0_addr, 0_addr}}  // [1] ADD [0], [0]
    });

  interpreter intr(i);

  SUBCASE("Repeated run invalidates cached values")
  {
    rz = 1.0;
    CHECK(real::base(intr.run()) == doctest::Approx(4.0));

    rz = 2.0;
    CHECK(real::base(intr.run()) == doctest::Approx(8.0));

    rz = -3.0;
    CHECK(real::base(intr.run()) == doctest::Approx(-12.0));
  }

  SUBCASE("Repeated run survives cache stamp wraparound")
  {
    for (unsigned j(0); j < 300; ++j)
    {
      rz = static_cast<double>(j);
      const auto ret(intr.run());
      CHECK(real::base(ret) == doctest::Approx(4.0 * rz));
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Rebind compatible individual")
{
  using namespace ultra;

  const gp::individual i1(
  {
    {f_add, {1.0, 2.0}},
    {f_add, {0_addr, 0_addr}}
  });

  const gp::individual i2(
  {
    {f_mul, {3.0, 4.0}},
    {f_add, {0_addr, 0_addr}}
  });

  interpreter intr(i1);

  CHECK(real::base(intr.run()) == doctest::Approx(6.0));

  intr.rebind(i2);

  CHECK(real::base(intr.run()) == doctest::Approx(24.0));
}

}  // TEST_SUITE
