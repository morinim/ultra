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

#include "kernel/gp/src/interpreter.h"
#include "kernel/gp/src/variable.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SRC::INTERPRETER")
{

TEST_CASE_FIXTURE(fixture1, "Run")
{
  using namespace ultra;

  src::variable x0(0, "X0");
  src::variable x1(1, "X1");

  SUBCASE("Simple")
  {
    gp::individual i(
    {
      {f_add, {&x0, &x1}}    // [0] ADD X0 X1
    });

    const auto ret(src::run(i, {1.0, 2.0}));
    CHECK(real::base(ret) == doctest::Approx(3.0));
  }

  SUBCASE("Mix")
  {
    gp::individual i(
      {
        {f_mul, {&x0, 2.0}},       // [0] MUL X0, $3.0
        {f_add, {&x0, &x1}},       // [1] ADD X0, X1
        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
      });

    for (unsigned j(0); j < 100; ++j)
    {
      const std::vector<value_t> input{random::between(-1000000.0, 1000000.0),
                                       random::between(-1000000.0, 1000000.0)};

      const auto ret(src::run(i, input));

      CHECK(real::base(ret) == doctest::Approx(std::get<D_DOUBLE>(input[1])
                                               - std::get<D_DOUBLE>(input[0])));
    }
  }
}

}  // TEST_SUITE("SRC::INTERPRETER")
