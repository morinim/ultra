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

#if !defined(ULTRA_FIXTURE3_H)
#define      ULTRA_FIXTURE3_H

#include "kernel/problem.h"

#include "kernel/gp/primitive/real.h"
#include "kernel/gp/primitive/string.h"

// Useful for multi categories tests.
struct fixture3
{
  struct Z final : public ultra::nullary
  {
    Z() : ultra::nullary("Z") {}

    [[nodiscard]] ultra::value_t eval() const override { return val; }

    double val {};
  };

  static constexpr ultra::D_DOUBLE x_val = 123.0;
  static constexpr ultra::D_DOUBLE y_val = 321.0;

  fixture3()
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
  ultra::nullary *z {prob.sset.insert<Z>()};

  ultra::function *f_abs {prob.sset.insert<ultra::real::abs>()};
  ultra::function *f_add {prob.sset.insert<ultra::real::add>()};
  ultra::function *f_aq {prob.sset.insert<ultra::real::aq>()};
  ultra::function *f_cos {prob.sset.insert<ultra::real::cos>()};
  ultra::function *f_div {prob.sset.insert<ultra::real::div>()};
  ultra::function *f_idiv {prob.sset.insert<ultra::real::idiv>()};
  ultra::function *f_ife {prob.sset.insert<ultra::real::ife>()};
  ultra::function *f_ifz {prob.sset.insert<ultra::real::ifz>()};
  ultra::function *f_ln {prob.sset.insert<ultra::real::ln>()};
  ultra::function *f_max {prob.sset.insert<ultra::real::max>()};
  ultra::function *f_mul {prob.sset.insert<ultra::real::mul>()};
  ultra::function *f_sigmoid {prob.sset.insert<ultra::real::sigmoid>()};
  ultra::function *f_sin {prob.sset.insert<ultra::real::sin>()};
  ultra::function *f_sqrt {prob.sset.insert<ultra::real::sqrt>()};
  ultra::function *f_sub {prob.sset.insert<ultra::real::sub>()};

  ultra::str::literal *s1 {prob.sset.insert<ultra::str::literal>("hello", 1)};
  ultra::str::literal *s2 {prob.sset.insert<ultra::str::literal>("world", 1)};
  ultra::str::literal *s3 {prob.sset.insert<ultra::str::literal>(":-)", 1)};

  ultra::function *s_ife {prob.sset.insert<ultra::str::ife>(
      1, ultra::function::param_data_types{1, 1, 1, 1})};
  ultra::function *f_len {prob.sset.insert<ultra::real::length>(
      0, ultra::function::param_data_types{1})};
};

#endif  // include guard
