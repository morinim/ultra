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

#include <numbers>

#include "kernel/gp/function.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

class debug_params : public ultra::function::params
{
public:
  debug_params(std::vector<ultra::value_t> v) : params_(std::move(v)) {}

  [[nodiscard]] ultra::value_t fetch_arg(std::size_t i) const override
  {
    return params_[i];
  }

  [[nodiscard]] ultra::value_t fetch_opaque_arg(std::size_t i) const override
  {
    return fetch_arg(i);
  }

private:
  std::vector<ultra::value_t> params_;
};

TEST_SUITE("FUNCTION")
{

TEST_CASE("REAL")
{
  using namespace ultra;
  using ultra::real::base;

  const D_DOUBLE inf(std::numeric_limits<D_DOUBLE>::infinity());
  const value_t empty;
/*
  SUBCASE("Abs")
  {
    real::abs f;

    CHECK(base(f.eval(debug_params({-1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({ 1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({ 0.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }
*/
  SUBCASE("Add")
  {
    real::add f;

    CHECK(base(f.eval(debug_params({-1.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({1.0, 1.0}))) == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({0.0, 10.0}))) == doctest::Approx(10.0));*/
    CHECK(!has_value(f.eval(debug_params({inf, -1.0}))));
    CHECK(!has_value(f.eval(debug_params({+inf, +inf}))));
    CHECK(!has_value(f.eval(debug_params({+inf, -inf}))));
/*    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
      CHECK(!has_value(f.eval(debug_params({0.0, {}}))));*/
  }
/*
  SUBCASE("AQ")
  {
    real::aq f;

    CHECK(base(f.eval(debug_params({1.0, 0.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({1.0, 10000.0})))
          == doctest::Approx(1.0/10000.0));
    CHECK(base(f.eval(debug_params({1.0, inf}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({inf, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, inf}))));
  }

  SUBCASE("Cos")
  {
    real::cos f;

    CHECK(base(f.eval(debug_params({0.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({std::numbers::pi})))
          == doctest::Approx(-1.0));
    CHECK(base(f.eval(debug_params({std::numbers::pi / 4.0})))
          == doctest::Approx(1.0 / std::sqrt(2.0)));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("Div")
  {
    real::div f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(base(f.eval(debug_params({-2.0, 2.0}))) == doctest::Approx(-1.0));
    CHECK(base(f.eval(debug_params({1.0, inf}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({inf, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, inf}))));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));
  }

  SUBCASE("GT")
  {
    real::gt f(0, {1, 1});

    CHECK(std::get<D_INT>(f.eval(debug_params({0.0, 1.0}))) == 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({1.0, 0.0}))) != 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({inf, 0.0}))) != 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({+inf, -inf}))) != 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({inf, inf}))) == 0);
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
    CHECK(!has_value(f.eval(debug_params({inf, {}}))));
  }

  SUBCASE("IDiv")
  {
    real::idiv f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, 2.0}))));
    CHECK(base(f.eval(debug_params({9.0, 4.0}))) == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({9.0, inf}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));
  }

  SUBCASE("IfE")
  {
    real::ife f;

    CHECK(base(f.eval(debug_params({0.0, 1.0, 2.0, 3.0})))
          == doctest::Approx(3.0));
    CHECK(base(f.eval(debug_params({1.0, 1.0, 2.0, 3.0})))
          == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({inf, inf, 2.0, 3.0})))
          == doctest::Approx(3.0));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0, 1.0, 2.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}, 1.0, 2.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, 0.0, {}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, 1.0, 2.0, {}}))));
  }

  SUBCASE("IfL")
  {
    real::ifl f;

    CHECK(base(f.eval(debug_params({0.0, 1.0, 2.0, 3.0})))
          == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({-inf, +inf, 2.0, 3.0})))
          == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({1.0, 0.0, 2.0, 3.0})))
          == doctest::Approx(3.0));
    CHECK(base(f.eval(debug_params({1.0, inf, 2.0, 3.0})))
          == doctest::Approx(2.0));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0, 1.0, 2.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}, 1.0, 2.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, 1.0, {}, 2.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0, 2.0, {}}))));
  }

  SUBCASE("IfZ")
  {
    real::ifz f;

    CHECK(base(f.eval(debug_params({0.0, 1.0, 2.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({1.0, 0.0, 2.0}))) == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({inf, 0.0, 2.0}))) == doctest::Approx(2.0));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0, 1.0}))));
    CHECK(base(f.eval(debug_params({1.0, {}, 2.0}))) == doctest::Approx(2.0));
    CHECK(!has_value(f.eval(debug_params({0.0, {}, 1.0}))));
  }

  SUBCASE("Length")
  {
    real::length f(0, {1});

    CHECK(base(f.eval(debug_params({"HELLO"}))) == doctest::Approx(5.0));
    CHECK(base(f.eval(debug_params({""}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("Ln")
  {
    real::ln f;

    CHECK(base(f.eval(debug_params({1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({std::numbers::e})))
          == doctest::Approx(1.0));
    CHECK(!has_value(f.eval(debug_params({0.0}))));
    CHECK(!has_value(f.eval(debug_params({inf}))));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("LT")
  {
    real::lt f(0, {1, 1});

    CHECK(std::get<D_INT>(f.eval(debug_params({0.0, 1.0}))) != 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({1.0, 0.0}))) == 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({-inf, +inf}))) != 0);
    CHECK(std::get<D_INT>(f.eval(debug_params({10.0, inf}))) != 0);
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
    CHECK(!has_value(f.eval(debug_params({inf, {}}))));
  }

  SUBCASE("Max")
  {
    real::max f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({1.0, 0.0}))) == doctest::Approx(1.0));
    CHECK(!has_value(f.eval(debug_params({inf, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("Mod")
  {
    real::mod f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({5.0, 2.0}))) == doctest::Approx(1.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, 2.0}))));
    CHECK(base(f.eval(debug_params({-2.0, 2.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));
  }

  SUBCASE("Mul")
  {
    real::mul f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({-2.0, 2.0}))) == doctest::Approx(-4.0));
    CHECK(!has_value(f.eval(debug_params({inf, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, inf}))));
    CHECK(!has_value(f.eval(debug_params({inf, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));
  }

  SUBCASE("Sin")
  {
    real::sin f;

    CHECK(base(f.eval(debug_params({0.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({std::numbers::pi})))
          == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({std::numbers::pi / 4.0})))
          == doctest::Approx(1.0 / std::sqrt(2.0)));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("SqRt")
  {
    real::sqrt f;

    CHECK(base(f.eval(debug_params({0.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({4.0}))) == doctest::Approx(2.0));
    CHECK(std::isinf(base(f.eval(debug_params({inf})))));
    CHECK(!has_value(f.eval(debug_params({-1.0}))));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("Sub")
  {
    real::sub f;

    CHECK(base(f.eval(debug_params({-1.0, 1.0}))) == doctest::Approx(-2.0));
    CHECK(base(f.eval(debug_params({1.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({0.0, 10.0}))) == doctest::Approx(-10.0));
    CHECK(!has_value(f.eval(debug_params({inf, -1.0}))));
    CHECK(!has_value(f.eval(debug_params({+inf, -inf}))));
    CHECK(!has_value(f.eval(debug_params({+inf, +inf}))));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("Sigmoid")
  {
    real::sigmoid f;

    CHECK(base(f.eval(debug_params({0.0}))) == doctest::Approx(0.5));
    CHECK(base(f.eval(debug_params({inf}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({-inf}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }
  */
}

TEST_CASE("INTEGER")
{
  using namespace ultra;
  using ultra::integer::base;

  const value_t empty;

  SUBCASE("Add")
  {
    integer::add f;

    CHECK(base(f.eval(debug_params({-1, 1}))) == 0);
    CHECK(base(f.eval(debug_params({1, 1}))) == 2);
    CHECK(base(f.eval(debug_params({0, 10}))) == 10);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::max(), 1})))
          == std::numeric_limits<D_INT>::max());
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), -1})))
          == std::numeric_limits<D_INT>::min());
  }

  SUBCASE("Div")
  {
    integer::div f;

    CHECK(base(f.eval(debug_params({0, 1}))) == 0);
    CHECK(base(f.eval(debug_params({1, 0}))) == 1);
    CHECK(base(f.eval(debug_params({10, 2}))) == 5);
    CHECK(base(f.eval(debug_params({10, -2}))) == -5);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), -1})))
          == std::numeric_limits<D_INT>::min());
  }

  SUBCASE("IfE")
  {
    integer::ife f;

    CHECK(base(f.eval(debug_params({0, 1, 2, 3}))) == 3);
    CHECK(base(f.eval(debug_params({-1, -1, 0, 1}))) == 0);
  }

  SUBCASE("IfL")
  {
    integer::ifl f;

    CHECK(base(f.eval(debug_params({0, 1, 2, 3}))) == 2);
    CHECK(base(f.eval(debug_params({0, 0, 1, 2}))) == 2);
    CHECK(base(f.eval(debug_params({1, 0, 2, 3}))) == 3);
  }

  SUBCASE("IfZ")
  {
    integer::ifz f;

    CHECK(base(f.eval(debug_params({0, 1, 2}))) == 1);
    CHECK(base(f.eval(debug_params({1, 0, 2}))) == 2);
    CHECK(base(f.eval(debug_params({-1, 0, 2}))) == 2);
  }

  SUBCASE("Mod")
  {
    integer::mod f;

    CHECK(base(f.eval(debug_params({0, 1}))) == 0);
    CHECK(base(f.eval(debug_params({5, 2}))) == 1);
    CHECK(base(f.eval(debug_params({1, 0}))) == 0);
    CHECK(base(f.eval(debug_params({-2, 2}))) == 0);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), -1})))
          == -1);
  }

  SUBCASE("Mul")
  {
    integer::mul f;

    CHECK(base(f.eval(debug_params({0, 1}))) == 0);
    CHECK(base(f.eval(debug_params({-2, 2}))) == -4);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::max(),
                                    std::numeric_limits<D_INT>::max()})))
          == std::numeric_limits<D_INT>::max());
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), 2})))
          == std::numeric_limits<D_INT>::min());
  }

  SUBCASE("Shl")
  {
    integer::shl f;

    CHECK(base(f.eval(debug_params({0, 10}))) == 0);
    CHECK(base(f.eval(debug_params({2, 1}))) == 4);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::max(),
                                    std::numeric_limits<D_INT>::max()})))
          == std::numeric_limits<D_INT>::max());
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), 2})))
          == std::numeric_limits<D_INT>::min());
  }

  SUBCASE("Sub")
  {
    integer::sub f;

    CHECK(base(f.eval(debug_params({-1, 1}))) == -2);
    CHECK(base(f.eval(debug_params({1, 1}))) == 0);
    CHECK(base(f.eval(debug_params({0, 10}))) == -10);
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::min(), 1})))
          == std::numeric_limits<D_INT>::min());
    CHECK(base(f.eval(debug_params({std::numeric_limits<D_INT>::max(), -1})))
          == std::numeric_limits<D_INT>::max());
  }
}

}  // TEST_SUITE("FUNCTION")
