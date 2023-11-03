/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <sstream>

#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/real.h"
#include "utility/log.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

struct fixture
{
  struct Z final : public ultra::terminal
  {
    Z() : ultra::terminal("Z") {}

    [[nodiscard]] ultra::value_t instance() const override { return val; }

    double val;
  };

  static constexpr ultra::D_DOUBLE x_val = 123.0;
  static constexpr ultra::D_DOUBLE y_val = 321.0;

  fixture() : f_add(prob.sset.insert<ultra::real::add>()),
              f_aq(prob.sset.insert<ultra::real::aq>()),
              f_cos(prob.sset.insert<ultra::real::cos>()),
              f_div(prob.sset.insert<ultra::real::div>()),
              f_idiv(prob.sset.insert<ultra::real::idiv>()),
              f_ife(prob.sset.insert<ultra::real::ife>()),
              f_ifz(prob.sset.insert<ultra::real::ifz>()),
              f_ln(prob.sset.insert<ultra::real::ln>()),
              f_max(prob.sset.insert<ultra::real::max>()),
              f_mul(prob.sset.insert<ultra::real::mul>()),
              f_sigmoid(prob.sset.insert<ultra::real::sigmoid>()),
              f_sin(prob.sset.insert<ultra::real::sin>()),
              f_sqrt(prob.sset.insert<ultra::real::sqrt>()),
              f_sub(prob.sset.insert<ultra::real::sub>())
  {
    prob.env.init().slp.code_length = 32;
  }

  ultra::problem prob {};

  ultra::real::literal *c0 {prob.sset.insert<ultra::real::literal>(0.0)};
  ultra::real::literal *c1 {prob.sset.insert<ultra::real::literal>(1.0)};
  ultra::real::literal *c2 {prob.sset.insert<ultra::real::literal>(2.0)};
  ultra::real::literal *c3 {prob.sset.insert<ultra::real::literal>(3.0)};
  ultra::real::literal *x {prob.sset.insert<ultra::real::literal>(x_val)};
  ultra::real::literal *neg_x {prob.sset.insert<ultra::real::literal>(-x_val)};
  ultra::real::literal *y {prob.sset.insert<ultra::real::literal>(y_val)};
  ultra::terminal *z {prob.sset.insert<Z>()};

  ultra::function *f_abs {prob.sset.insert<ultra::real::abs>()};
  ultra::function *f_add;
  ultra::function *f_aq;
  ultra::function *f_cos;
  ultra::function *f_div;
  ultra::function *f_idiv;
  ultra::function *f_ife;
  ultra::function *f_ifz;
  ultra::function*f_ln;
  ultra::function *f_max;
  ultra::function *f_mul;
  ultra::function *f_sigmoid;
  ultra::function *f_sin;
  ultra::function *f_sqrt;
  ultra::function *f_sub;
};

TEST_SUITE("GP INDIVIDUAL")
{

TEST_CASE_FIXTURE(fixture, "Random creation")
{
  using namespace ultra;

  log::reporting_level = log::lALL;

  // Variable length random creation.
  for (auto l(prob.sset.categories() + 2); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual i(prob);

    CHECK(i.is_valid());
    CHECK(i.size() == l);
    CHECK(i.age() == 0);
  }
}

}  // TEST_SUITE("GP INDIVIDUAL")
