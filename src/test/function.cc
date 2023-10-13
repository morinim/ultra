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

#include <numbers>

#include "kernel/gp/function.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

class debug_params : public ultra::function::params
{
public:
  debug_params(const std::vector<ultra::value_t> &v) : params_(v) {}

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

  SUBCASE("Abs")
  {
    real::abs f;

    CHECK(base(f.eval(debug_params({-1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({ 1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({ 0.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({inf}))));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("Add")
  {
    real::add f;

    CHECK(base(f.eval(debug_params({-1.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({1.0, 1.0}))) == doctest::Approx(2.0));
    CHECK(base(f.eval(debug_params({0.0, 10.0}))) == doctest::Approx(10.0));
    CHECK(!has_value(f.eval(debug_params({inf, -1.0}))));
    CHECK(!has_value(f.eval(debug_params({+inf, -inf}))));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("AQ")
  {
    real::aq f;

    CHECK(base(f.eval(debug_params({1.0, 0.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({1.0, 10000.0})))
          == doctest::Approx(1.0/10000.0));
    CHECK(!has_value(f.eval(debug_params({1.0, inf}))));
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
    CHECK(!has_value(f.eval(debug_params({inf}))));
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("Div")
  {
    real::div f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(base(f.eval(debug_params({-2.0, 2.0}))) == doctest::Approx(-1.0));
    CHECK(!has_value(f.eval(debug_params({1.0, inf}))));
    CHECK(!has_value(f.eval(debug_params({inf, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({inf, inf}))));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));
  }

  SUBCASE("GT")
  {
    real::gt f(0, {1, 1});

    CHECK(std::get<D_INT>(f.eval(debug_params({0.0, 1.0}))) == false);
    CHECK(std::get<D_INT>(f.eval(debug_params({1.0, 0.0}))) == true);
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("IDiv")
  {
    real::idiv f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(base(f.eval(debug_params({9.0, 4.0}))) == doctest::Approx(2.0));
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
    CHECK(base(f.eval(debug_params({1.0, 0.0, 2.0, 3.0})))
          == doctest::Approx(3.0));
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
    CHECK(!has_value(f.eval(debug_params({empty}))));
  }

  SUBCASE("GT")
  {
    real::lt f(0, {1, 1});

    CHECK(std::get<D_INT>(f.eval(debug_params({0.0, 1.0}))) == true);
    CHECK(std::get<D_INT>(f.eval(debug_params({1.0, 0.0}))) == false);
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("Max")
  {
    real::max f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(1.0));
    CHECK(base(f.eval(debug_params({1.0, 0.0}))) == doctest::Approx(1.0));
    CHECK(!has_value(f.eval(debug_params({{}, 0.0}))));
    CHECK(!has_value(f.eval(debug_params({0.0, {}}))));
  }

  SUBCASE("Mod")
  {
    real::mod f;

    CHECK(base(f.eval(debug_params({0.0, 1.0}))) == doctest::Approx(0.0));
    CHECK(base(f.eval(debug_params({5.0, 2.0}))) == doctest::Approx(1.0));
    CHECK(!has_value(f.eval(debug_params({1.0, 0.0}))));
    CHECK(base(f.eval(debug_params({-2.0, 2.0}))) == doctest::Approx(0.0));
    CHECK(!has_value(f.eval(debug_params({{}, 1.0}))));
    CHECK(!has_value(f.eval(debug_params({1.0, {}}))));

  }
}

}  // TEST_SUITE("FUNCTION")
