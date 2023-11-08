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

  fixture()
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
};

TEST_SUITE("GP INDIVIDUAL")
{

TEST_CASE_FIXTURE(fixture, "Random creation")
{
  using namespace ultra;

  // Variable length random creation.
  for (auto l(1); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(ind.size() == l);
    CHECK(!ind.empty());
    CHECK(ind.age() == 0);

    for (locus::index_t i(0); i < ind.size(); ++i)
      for (symbol::category_t c(0); c < prob.sset.categories(); ++c)
      {
        CHECK(ind[{i, c}].category() == c);

        for (const auto &a : ind[{i, c}].args)
          if (const auto *pa(std::get_if<param_address>(&a)); pa)
            CHECK(as_integer(*pa) < i);
      }
  }
}

TEST_CASE_FIXTURE(fixture, "Construction from vector")
{
  using namespace ultra;

  gp::individual i({
                     {f_add, {3.0, 2.0}},       // [0] ADD $3.0, $2.0
                     {f_add, {0_addr, 1.0}},    // [1] ADD [0], $1.0
                     {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
                   });

  CHECK(i.is_valid());
  CHECK(i.size() == 3);
  CHECK(!i.empty());
  CHECK(i.age() == 0);

  CHECK(i[{0, 0}].category() == symbol::default_category);
  CHECK(i[{1, 0}].category() == symbol::default_category);
  CHECK(i[{2, 0}].category() == symbol::default_category);

  CHECK(i[{0, 0}].func == f_add);
  CHECK(i[{1, 0}].func == f_add);
  CHECK(i[{2, 0}].func == f_sub);

  CHECK(i[{2, 0}].args == gene::arg_pack{1_addr, 0_addr});
}

}  // TEST_SUITE("GP INDIVIDUAL")
