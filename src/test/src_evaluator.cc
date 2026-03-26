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

#include "test/debug_datasets.h"

#include "kernel/gp/src/evaluator.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/problem.h"
#include "kernel/gp/src/variable.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("src::evaluator")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  CHECK(src::ExampleEvaluator<src::mae_error_functor<gp::individual>,
        src::dataframe, gp::individual>);
  CHECK(src::ExampleEvaluator<src::rmae_error_functor<gp::individual>,
        src::dataframe, gp::individual>);
  CHECK(src::ExampleEvaluator<src::mse_error_functor<gp::individual>,
        src::dataframe, gp::individual>);
  CHECK(src::ExampleEvaluator<src::count_error_functor<gp::individual>,
        src::dataframe, gp::individual>);

  CHECK(src::ExampleEvaluator<src::mae_error_functor<gp::individual>,
        src::multi_dataset<src::dataframe>, gp::individual>);
  CHECK(src::ExampleEvaluator<src::rmae_error_functor<gp::individual>,
        src::multi_dataset<src::dataframe>, gp::individual>);
  CHECK(src::ExampleEvaluator<src::mse_error_functor<gp::individual>,
        src::multi_dataset<src::dataframe>, gp::individual>);
  CHECK(src::ExampleEvaluator<src::count_error_functor<gp::individual>,
        src::multi_dataset<src::dataframe>, gp::individual>);

  CHECK(Evaluator<src::mae_evaluator<gp::individual>>);
  CHECK(Evaluator<src::mae_evaluator<gp::team<gp::individual>>>);
  CHECK(Evaluator<src::rmae_evaluator<gp::individual>>);
  CHECK(Evaluator<src::rmae_evaluator<gp::team<gp::individual>>>);
  CHECK(Evaluator<src::mse_evaluator<gp::individual>>);
  CHECK(Evaluator<src::mse_evaluator<gp::team<gp::individual>>>);
  CHECK(Evaluator<src::count_error_evaluator<gp::individual>>);
  CHECK(Evaluator<src::count_error_evaluator<gp::team<gp::individual>>>);
}

[[nodiscard]] ultra::src::problem make_problem(std::istream &is)
{
  ultra::src::problem pr(is);
  pr.params.init();
  pr.setup_symbols();
  return pr;
}

TEST_CASE("Aggregate evaluators")
{
  using namespace ultra;

  std::istringstream sr(R"(
        95.2425,  2.81
      1554,       6
      2866.5485,  7.043
      4680,       8
     11110,      10
     18386.0340, 11.38
     22620,      12
     41370,      14
     54240,      15
    168420,      20
)");

  auto pr(make_problem(sr));
  CHECK(!pr.data.selected().empty());

  const auto *x1(static_cast<const src::variable *>(pr.sset.decode("X1")));
  CHECK(x1);

  const function *f_ife(pr.insert<ultra::real::ife>());

  const std::vector out = {95.2425, 1554.0, 2866.5485, 4680.0, 11110.0,
                           18386.0340, 22620.0, 41370.0, 54240.0, 168420.0};

  const auto make_individual([&](auto transform)
  {
    return gp::individual({
        {f_ife, {x1, 15.000, transform(out[8]), transform(out[9])}},
        {f_ife, {x1, 14.000, transform(out[7]), 0_addr}},
        {f_ife, {x1, 12.000, transform(out[6]), 1_addr}},
        {f_ife, {x1, 11.380, transform(out[5]), 2_addr}},
        {f_ife, {x1, 10.000, transform(out[4]), 3_addr}},
        {f_ife, {x1,  8.000, transform(out[3]), 4_addr}},
        {f_ife, {x1,  7.043, transform(out[2]), 5_addr}},
        {f_ife, {x1,  6.000, transform(out[1]), 6_addr}},
        {f_ife, {x1,  2.810, transform(out[0]), 7_addr}}
      });
  });

  const auto delphi(make_individual([](double v){ return v; }));
  const auto delta1(make_individual([](double v){ return v + 1.0; }));
  const auto delta2(make_individual([](double v){ return v + 2.0; }));

  const auto huge1(make_individual([&](const double &v)
  {
    return (&v == &out[0]) ? HUGE_VAL : v;
  }));

  const auto huge2(make_individual([&](const double &v)
  {
    if (&v != &out[0] && &v != &out[1])
      return v;

    return (&v == &out[0]) ? HUGE_VAL : -HUGE_VAL;
  }));

  using std::ranges::views::zip;

  const auto inputs =
    pr.data.selected()
    | std::views::transform([](const auto &e) { return e.input; });

  SUBCASE("Delphi knows everything")
  {
    {
      const src::reg_oracle oracle(delphi);

      for (auto [input, expected] : zip(inputs, out))
        CHECK(std::get<D_DOUBLE>(oracle(input)) == doctest::Approx(expected));
    }

    {
      const src::reg_oracle oracle(delta1);

      for (auto [input, expected] : zip(inputs, out))
        CHECK(std::get<D_DOUBLE>(oracle(input)) == doctest::Approx(expected+1));
    }

    {
      const src::reg_oracle oracle(delta2);

      for (auto [input, expected] : zip(inputs, out))
        CHECK(std::get<D_DOUBLE>(oracle(input)) == doctest::Approx(expected+2));
    }
  }

  SUBCASE("mae_evaluator")
  {
    src::mae_evaluator<gp::individual> mae(pr.data);
    CHECK(mae(delphi) == doctest::Approx(0.0));
    CHECK(mae(delta1) == doctest::Approx(-1.0));
    CHECK(mae(delta2) == doctest::Approx(-2.0));
    CHECK(std::isnan(mae(huge1)));
    CHECK(std::isnan(mae(huge2)));
  }

  SUBCASE("rmae_evaluator")
  {
    src::rmae_evaluator<gp::individual> rmae(pr.data);
    CHECK(rmae(delphi) == doctest::Approx(0.0));
    CHECK(rmae(delta1) == doctest::Approx(-0.118876));
    CHECK(rmae(delta2) == doctest::Approx(-0.23666));
    CHECK(std::isnan(rmae(huge1)));
    CHECK(std::isnan(rmae(huge2)));
  }

  SUBCASE("mse_evaluator")
  {
    src::mse_evaluator<gp::individual> mse(pr.data);
    CHECK(mse(delphi) == doctest::Approx(0.0));
    CHECK(mse(delta1) == doctest::Approx(-1.0));
    CHECK(mse(delta2) == doctest::Approx(-4.0));
    CHECK(std::isnan(mse(huge1)));
    CHECK(std::isnan(mse(huge2)));
  }

  SUBCASE("count_error_evaluator")
  {
    src::count_error_evaluator<gp::individual> count(pr.data);
    CHECK(count(delphi) == doctest::Approx(0.0));
    CHECK(count(delta1) == doctest::Approx(-10.0));
    CHECK(count(delta2) == doctest::Approx(-10.0));
    CHECK(count(huge1) == doctest::Approx(-1.0));
    CHECK(count(huge2) == doctest::Approx(-2.0));
  }
}

TEST_CASE("binary_evaluator")
{
  using namespace ultra;

  std::istringstream is(debug::gender_trick);
  auto pr(make_problem(is));
  CHECK(!pr.data.selected().empty());

  const auto *easy(static_cast<const src::variable *>(
                     pr.sset.decode("EASY")));
  REQUIRE(easy);

  const function *f_add(pr.insert<ultra::real::add>());

  SUBCASE("perfect case")
  {
    const gp::individual delphi(
    {
      {f_add, {easy, easy}}
    });

    src::binary_evaluator<gp::individual> eva(pr.data);

    CHECK(eva(delphi) == doctest::Approx(0.0));
  }

  SUBCASE("failing individual")
  {
    const gp::individual wrong(
    {
      {f_add, {easy, 10.0}}
    });

    src::binary_evaluator<gp::individual> eva(pr.data);

    CHECK(eva(wrong) < 0.0);
  }
}

TEST_CASE("fast evaluation matches full evaluation for constant error")
{
  using namespace ultra;

  std::ostringstream os;
  for (int i(0); i < 100; ++i)
    os << i << ".0," << i << ".0\n";

  std::istringstream is(os.str());
  auto pr(make_problem(is));

  const auto *x1(static_cast<const src::variable *>(pr.sset.decode("X1")));
  REQUIRE(x1);

  const function *f_add(pr.insert<ultra::real::add>());

  const gp::individual plus_one({{f_add, {x1, 1.0}}});

  SUBCASE("average evaluator")
  {
    src::mae_evaluator<gp::individual> eva(pr.data);
    CHECK(eva(plus_one) == doctest::Approx(-1.0));
    CHECK(eva.fast(plus_one) == doctest::Approx(eva(plus_one)));
  }

  SUBCASE("sum evaluator")
  {
    src::sum_error_evaluator<gp::individual,
                             src::mae_error_functor<gp::individual>>
      eva(pr.data);
    CHECK(eva(plus_one) == doctest::Approx(-100.0));
    CHECK(eva.fast(plus_one) == doctest::Approx(eva(plus_one)));
  }
}

template<class E> concept HasOracle =
  requires(const E &e, const ultra::gp::individual &i)
{
  e.oracle(i);
};

TEST_CASE("aggregate evaluator oracle availability")
{
  using namespace ultra;

  using mae_t = src::mae_evaluator<gp::individual>;
  using rmae_t = src::rmae_evaluator<gp::individual>;
  using mse_t = src::mse_evaluator<gp::individual>;
  using count_t = src::count_error_evaluator<gp::individual>;

  CHECK(HasOracle<mae_t>);
  CHECK(HasOracle<rmae_t>);
  CHECK(HasOracle<mse_t>);
  CHECK_FALSE(HasOracle<count_t>);
}

TEST_CASE("EvaluationDataset concept")
{
  using namespace ultra;

  CHECK((src::EvaluationDataset<src::dataframe>));
  CHECK((src::EvaluationDataset<src::multi_dataset<src::dataframe>>));

  CHECK_FALSE((src::EvaluationDataset<int>));
}

TEST_CASE("team evaluator with identical members")
{
  using namespace ultra;

  std::istringstream is(R"(
    1.0,1.0
    2.0,2.0
    3.0,3.0
  )");

  auto pr(make_problem(is));

  const auto *x1(static_cast<const src::variable *>(pr.sset.decode("X1")));
  REQUIRE(x1);

  const function *f_add(pr.insert<ultra::real::add>());
  const gp::individual id({{f_add, {x1, 0.0}}});

  gp::team<gp::individual> team({id, id, id});

  src::mae_evaluator<gp::team<gp::individual>> eva(pr.data);

  CHECK(eva(team) == doctest::Approx(0.0));
}

}  // TEST_SUITE
