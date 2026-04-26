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

#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/search.h"
#include "kernel/de/numerical_refiner.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <sstream>
#include <utility>
#include <vector>

TEST_SUITE("src::search")
{

TEST_CASE("Symbolic regression - single variable")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  // The target function is `x + sin(x)`.
  std::istringstream training(R"(
    -9.456,-10.0
    -8.989, -8.0
    -5.721, -6.0
    -3.243, -4.0
    -2.909, -2.0
     0.000,  0.0
     2.909,  2.0
     3.243,  4.0
     5.721,  6.0
     8.989,  8.0
  )");

  // Reading input data.
  src::problem prob(training);
  CHECK(prob.variables() == 1);
  CHECK(!prob.classification());

  // Setting up symbols.
  prob.insert<real::sin>();
  prob.insert<real::cos>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::div>();
  prob.insert<real::mul>();
  CHECK(prob.ready());

  // Searching.
  src::search s(prob);
  const auto result(s.run(8));

  const auto holder(s.oracle(result.best_individual()));
  const auto &oracle(*holder);

  REQUIRE(oracle.is_valid());

  const std::vector<std::pair<double, double>> test =
  {
    {-3.2431975, -4.0},
    {-2.9092974, -2.0},
    { 0.0000000,  0.0},
    { 2.9092974,  2.0},
    { 3.2431975,  4.0}
  };

  for (const auto &[out, in] : test)
  {
    const auto v(oracle({in}));
    REQUIRE(has_value(v));
    CHECK(std::get<D_DOUBLE>(v) == doctest::Approx(out).epsilon(1e-4));
  }
}

TEST_CASE("Symbolic regression - multiple variables")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  // The target function is `ln(x*x + y*y)`
  std::istringstream training(R"(
    -2.079, 0.25, 0.25
    -0.693, 0.50, 0.50
     0.693, 1.00, 1.00
     0.000, 0.00, 1.00
     0.000, 1.00, 0.00
     1.609, 1.00, 2.00
     1.609, 2.00, 1.00
     2.079, 2.00, 2.00
  )");

  // Reading input data.
  src::problem prob(training);
  CHECK(prob.variables() == 2);
  CHECK(!prob.classification());

  // Setting up symbols.
  prob.insert<real::sin>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::mul>();
  prob.insert<real::ln>();
  CHECK(prob.ready());

  // Searching.
  src::search s(prob);
  const auto result(s.run(8));

  const auto holder(s.oracle(result.best_individual()));
  const auto &oracle(*holder);

  REQUIRE(oracle.is_valid());

  const std::vector<std::pair<double, std::vector<value_t>>> test =
  {
    { 2.07944, { 2.0, 2.0}},
    { 1.60944, { 2.0, 1.0}},
    { 1.60944, { 1.0, 2.0}},
    { 0.00000, { 1.0, 0.0}},
    { 0.69315, { 1.0, 1.0}}
  };

  for (const auto &[out, in] : test)
  {
    const auto v(oracle(in));
    REQUIRE(has_value(v));
    CHECK(std::get<D_DOUBLE>(v) == doctest::Approx(out).epsilon(1e-4));
  }
}

TEST_CASE("Refinement callback dispatches by problem type")
{
  using namespace ultra;

  log::reporting_level = log::lWARNING;

  SUBCASE("Regression evaluator")
  {
    std::istringstream training(R"(
      -1.0, -1.0
       0.0,  0.0
       1.0,  1.0
    )");

    src::problem prob(training);
    prob.setup_symbols();

    src::search s(prob);

    bool called(false);
    s.refinement([&called]<Evaluator E>(
                   gp::individual &, const E &,
                   const parameters::refinement_parameters &)
                 {
                   called = std::same_as<
                     E, evaluator_proxy<src::search<>::reg_evaluator_t>>;

                   return std::optional<double>();
                 });

    prob.params.refinement.fraction = 1.0;
    prob.params.evolution.generations = 1;

    s.run(1);

    CHECK(called);
  }

  SUBCASE("Classification evaluator")
  {
    std::istringstream training(R"(
      A, 0.0
      B, 1.0
      A, 0.1
      B, 0.9
    )");

    src::problem prob(training);
    CHECK(prob.classification());
    prob.setup_symbols();

    src::search s(prob);

    bool called(false);
    s.refinement([&called]<Evaluator E>(
                   gp::individual &, const E &,
                   const parameters::refinement_parameters &)
                 {
                   called = std::same_as<
                     E, evaluator_proxy<src::search<>::class_evaluator_t>>;

                   return std::optional<double>();
                 });

    prob.params.refinement.fraction = 1.0;
    prob.params.evolution.generations = 1;

    s.run(1);

    CHECK(called);
  }
}

}  // TEST_SUITE
