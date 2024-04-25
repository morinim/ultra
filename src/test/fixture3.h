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
    prob.params.init().slp.code_length = 32;
  }

  ultra::problem prob {};

  ultra::real::literal *c0 {prob.insert<ultra::real::literal>(0.0)};
  ultra::real::literal *c1 {prob.insert<ultra::real::literal>(1.0)};
  ultra::real::literal *c2 {prob.insert<ultra::real::literal>(2.0)};
  ultra::real::literal *c3 {prob.insert<ultra::real::literal>(3.0)};
  ultra::real::literal *x {prob.insert<ultra::real::literal>(x_val)};
  ultra::real::literal *neg_x {prob.insert<ultra::real::literal>(-x_val)};
  ultra::real::literal *y {prob.insert<ultra::real::literal>(y_val)};
  ultra::nullary *z {prob.insert<Z>()};

  ultra::function *f_abs {prob.insert<ultra::real::abs>()};
  ultra::function *f_add {prob.insert<ultra::real::add>()};
  ultra::function *f_aq {prob.insert<ultra::real::aq>()};
  ultra::function *f_cos {prob.insert<ultra::real::cos>()};
  ultra::function *f_div {prob.insert<ultra::real::div>()};
  ultra::function *f_idiv {prob.insert<ultra::real::idiv>()};
  ultra::function *f_ife {prob.insert<ultra::real::ife>()};
  ultra::function *f_ifz {prob.insert<ultra::real::ifz>()};
  ultra::function *f_ln {prob.insert<ultra::real::ln>()};
  ultra::function *f_max {prob.insert<ultra::real::max>()};
  ultra::function *f_mul {prob.insert<ultra::real::mul>()};
  ultra::function *f_sigmoid {prob.insert<ultra::real::sigmoid>()};
  ultra::function *f_sin {prob.insert<ultra::real::sin>()};
  ultra::function *f_sqrt {prob.insert<ultra::real::sqrt>()};
  ultra::function *f_sub {prob.insert<ultra::real::sub>()};

  ultra::str::literal *s1 {prob.insert<ultra::str::literal>("hello", 1)};
  ultra::str::literal *s2 {prob.insert<ultra::str::literal>("world", 1)};
  ultra::str::literal *s3 {prob.insert<ultra::str::literal>(":-)", 1)};

  ultra::function *s_ife {prob.insert<ultra::str::ife>(
      1, ultra::function::param_data_types{1, 1, 1, 1})};
  ultra::function *f_len {prob.insert<ultra::real::length>(
      0, ultra::function::param_data_types{1})};
};

#endif  // include guard
