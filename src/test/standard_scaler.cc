/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/gp/src/standard_scaler.h"
#include "kernel/exceptions.h"
#include "kernel/gp/src/dataframe.h"
#include "test/debug_datasets.h"

#include <cmath>
#include <limits>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("src::standard_scaler")
{

TEST_CASE("initial state and construction")
{
  using namespace ultra;
  using src::standard_scaler;

  SUBCASE("default constructor")
  {
    const standard_scaler scaler;
    CHECK(!scaler.is_fitted());
    CHECK(scaler.is_valid());
  }

  SUBCASE("explicit options constructor")
  {
    const standard_scaler::options opt{.scale_inputs = false,
                                       .scale_output = true};
    const standard_scaler scaler(opt);
    CHECK(!scaler.is_fitted());
    CHECK(scaler.is_valid());
  }
}

TEST_CASE("fit and transform default options")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}, {"x1", d_double}});
  df.push_back({.output = 100.0, .input = {10.0, 1.0}});
  df.push_back({.output = 200.0, .input = {20.0, 3.0}});
  df.push_back({.output = 300.0, .input = {30.0, 5.0}});

  standard_scaler scaler;
  CHECK(!scaler.is_fitted());

  scaler.fit(df);
  CHECK(scaler.is_fitted());
  CHECK(scaler.is_valid());

  scaler.transform(df);
  const auto it(df.begin());

  // x0: mean = 20.0, sample std = 10.0 -> values: -1.0, 0.0, 1.0
  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(1.0));

  // x1: mean = 3.0, sample std = 2.0 -> values: -1.0, 0.0, 1.0
  CHECK(std::get<D_DOUBLE>(it[0].input[1]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[1]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[1]) == doctest::Approx(1.0));

  // Default options do not scale output
  CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(100.0));
  CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(200.0));
  CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(300.0));

  // When scale_output is disabled, inverse transform returns the input value
  CHECK(scaler.inverse_transform_target(150.0) == doctest::Approx(150.0));
  CHECK(scaler.inverse_transform_target(-50.0) == doctest::Approx(-50.0));
}

TEST_CASE("scale output enabled and inverse transform")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}});
  df.push_back({.output = 100.0, .input = {10.0}});
  df.push_back({.output = 200.0, .input = {20.0}});
  df.push_back({.output = 300.0, .input = {30.0}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);

  scaler.fit(df);
  CHECK(scaler.is_fitted());

  scaler.transform(df);
  const auto it(df.begin());

  // Output: mean = 200.0, sample std = 100.0 -> values: -1.0, 0.0, 1.0
  CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(1.0));

  // Inverse transform target checks
  CHECK(scaler.inverse_transform_target(-1.0) == doctest::Approx(100.0));
  CHECK(scaler.inverse_transform_target(0.0) == doctest::Approx(200.0));
  CHECK(scaler.inverse_transform_target(1.0) == doctest::Approx(300.0));
  CHECK(scaler.inverse_transform_target(2.5) == doctest::Approx(450.0));
  CHECK(scaler.inverse_transform_target(-0.5) == doctest::Approx(150.0));
}

TEST_CASE("selective scaling options")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  auto make_df([]()
  {
    dataframe df;
    df.set_schema({{"y", d_double}, {"x0", d_double}});
    df.push_back({.output = 100.0, .input = {10.0}});
    df.push_back({.output = 200.0, .input = {20.0}});
    df.push_back({.output = 300.0, .input = {30.0}});
    return df;
  });

  SUBCASE("scale inputs only")
  {
    auto df(make_df());
    const standard_scaler::options opt{.scale_inputs = true,
                                       .scale_output = false};
    standard_scaler scaler(opt);
    scaler.fit(df);
    scaler.transform(df);
    const auto it(df.begin());

    CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-1.0));
    CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
    CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(1.0));

    CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(100.0));
    CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(200.0));
    CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(300.0));
  }

  SUBCASE("scale output only")
  {
    auto df(make_df());
    const standard_scaler::options opt{.scale_inputs = false,
                                       .scale_output = true};
    standard_scaler scaler(opt);
    scaler.fit(df);
    scaler.transform(df);
    const auto it(df.begin());

    CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(10.0));
    CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(20.0));
    CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(30.0));

    CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(-1.0));
    CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(0.0));
    CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(1.0));
  }

  SUBCASE("neither scaled")
  {
    auto df(make_df());
    const standard_scaler::options opt{.scale_inputs = false,
                                       .scale_output = false};
    standard_scaler scaler(opt);
    scaler.fit(df);
    scaler.transform(df);
    const auto it(df.begin());

    CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(10.0));
    CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(20.0));
    CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(30.0));

    CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(100.0));
    CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(200.0));
    CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(300.0));

    CHECK(scaler.inverse_transform_target(42.0) == doctest::Approx(42.0));
  }
}

TEST_CASE("train test consistency")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe train_df;
  train_df.set_schema({{"y", d_double}, {"x0", d_double}});
  train_df.push_back({.output = 100.0, .input = {10.0}});
  train_df.push_back({.output = 200.0, .input = {20.0}});
  train_df.push_back({.output = 300.0, .input = {30.0}});

  standard_scaler scaler;
  scaler.fit(train_df);

  dataframe test_df;
  test_df.set_schema({{"y", d_double}, {"x0", d_double}});
  test_df.push_back({.output = 0.0, .input = {15.0}});
  test_df.push_back({.output = 0.0, .input = {20.0}});
  test_df.push_back({.output = 0.0, .input = {40.0}});

  scaler.transform(test_df);
  const auto it(test_df.begin());

  // Train mean = 20.0, std = 10.0
  // test[0]: (15 - 20) / 10 = -0.5
  // test[1]: (20 - 20) / 10 =  0.0
  // test[2]: (40 - 20) / 10 =  2.0
  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-0.5));
  CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(2.0));
}

TEST_CASE("constant columns protection")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}, {"x1", d_double}});
  df.push_back({.output = 42.0, .input = {7.0, 1.0}});
  df.push_back({.output = 42.0, .input = {7.0, 2.0}});
  df.push_back({.output = 42.0, .input = {7.0, 3.0}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);
  scaler.fit(df);
  scaler.transform(df);
  const auto it(df.begin());

  // Constant input column (variance 0 -> std_dev set to 1.0)
  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(0.0));

  // Non-constant input column scaled normally
  CHECK(std::get<D_DOUBLE>(it[0].input[1]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[1]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[1]) == doctest::Approx(1.0));

  // Constant output column
  CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].output) == doctest::Approx(0.0));

  // Inverting 0.0 yields original constant value
  CHECK(scaler.inverse_transform_target(0.0) == doctest::Approx(42.0));
}

TEST_CASE("small non-constant columns")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x", d_double}});
  df.push_back({.output = 0.0, .input = {0.0}});
  df.push_back({.output = 1e-13, .input = {1e-13}});
  df.push_back({.output = 2e-13, .input = {2e-13}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);
  scaler.fit(df);
  scaler.transform(df);

  const auto it(df.begin());
  for (unsigned i(0); i < 3; ++i)
  {
    const double expected(static_cast<double>(i) - 1.0);
    CHECK(std::get<D_DOUBLE>(it[i].input[0]) == doctest::Approx(expected));
    CHECK(std::get<D_DOUBLE>(it[i].output) == doctest::Approx(expected));
  }

  CHECK(scaler.inverse_transform_target(1.0) == doctest::Approx(2e-13));
}

TEST_CASE("constant fractional columns")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x", d_double}});
  for (unsigned i(0); i < 3; ++i)
    df.push_back({.output = 0.1, .input = {0.1}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);
  scaler.fit(df);
  scaler.transform(df);

  for (const auto &ex : df)
  {
    CHECK(std::abs(std::get<D_DOUBLE>(ex.input[0])) < 1e-30);
    CHECK(std::abs(std::get<D_DOUBLE>(ex.output)) < 1e-30);
  }
  CHECK(scaler.inverse_transform_target(0.0) == doctest::Approx(0.1));
}

TEST_CASE("extreme finite columns")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  const auto check_pair([](double low, double high)
  {
    dataframe df;
    df.set_schema({{"y", d_double}, {"x", d_double}});
    df.push_back({.output = low, .input = {low}});
    df.push_back({.output = high, .input = {high}});

    const standard_scaler::options opt{.scale_inputs = true,
                                       .scale_output = true};
    standard_scaler scaler(opt);
    scaler.fit(df);
    scaler.transform(df);

    const double expected(std::sqrt(0.5));
    const auto it(df.begin());
    CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-expected));
    CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(expected));
    CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(-expected));
    CHECK(std::get<D_DOUBLE>(it[1].output) == doctest::Approx(expected));
    CHECK(scaler.inverse_transform_target(std::get<D_DOUBLE>(it[0].output))
          / low == doctest::Approx(1.0));
    CHECK(scaler.inverse_transform_target(std::get<D_DOUBLE>(it[1].output))
          / high == doctest::Approx(1.0));
  });

  check_pair(1e-200, 2e-200);
  check_pair(1e200, 2e200);
  check_pair(1e308, 1.5e308);
  check_pair(1e308, std::nextafter(1e308,
                                  std::numeric_limits<double>::infinity()));
  check_pair(-1e308, 1e308);
}

TEST_CASE("reject mismatched dataframe schema")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe train;
  train.set_schema({{"y", d_double}, {"x0", d_double}, {"x1", d_double}});
  train.push_back({.output = 0.0, .input = {10.0, 100.0}});
  train.push_back({.output = 1.0, .input = {20.0, 200.0}});

  standard_scaler scaler;
  scaler.fit(train);

  dataframe test;
  test.set_schema({{"y", d_double}, {"x1", d_double}, {"x0", d_double}});
  test.push_back({.output = 0.0, .input = {150.0, 15.0}});

  CHECK_THROWS_AS(scaler.transform(test), exception::data_format);
  CHECK(std::get<D_DOUBLE>(test.front().input[0]) == doctest::Approx(150.0));
  CHECK(std::get<D_DOUBLE>(test.front().input[1]) == doctest::Approx(15.0));
}

TEST_CASE("single sample dataset")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}});
  df.push_back({.output = 99.0, .input = {5.0}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);
  scaler.fit(df);
  scaler.transform(df);
  const auto it(df.begin());

  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[0].output) == doctest::Approx(0.0));

  CHECK(scaler.inverse_transform_target(0.0) == doctest::Approx(99.0));
}

TEST_CASE("mixed data types")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_int},
                 {"x_double", d_double},
                 {"x_int", d_int},
                 {"x_string", d_string}});

  df.push_back({.output = 0, .input = {10.0, 100, std::string("red")}});
  df.push_back({.output = 1, .input = {20.0, 200, std::string("green")}});
  df.push_back({.output = 0, .input = {30.0, 300, std::string("blue")}});

  const standard_scaler::options opt{.scale_inputs = true,
                                     .scale_output = true};
  standard_scaler scaler(opt);
  scaler.fit(df);
  scaler.transform(df);
  const auto it(df.begin());

  // Double input column is standardized
  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(1.0));

  // Integer and string inputs remain unchanged
  CHECK(std::get<D_INT>(it[0].input[1]) == 100);
  CHECK(std::get<D_INT>(it[1].input[1]) == 200);
  CHECK(std::get<D_INT>(it[2].input[1]) == 300);

  CHECK(std::get<D_STRING>(it[0].input[2]) == "red");
  CHECK(std::get<D_STRING>(it[1].input[2]) == "green");
  CHECK(std::get<D_STRING>(it[2].input[2]) == "blue");

  // Non-double target is untouched
  CHECK(std::get<D_INT>(it[0].output) == 0);
  CHECK(std::get<D_INT>(it[1].output) == 1);
  CHECK(std::get<D_INT>(it[2].output) == 0);
}

TEST_CASE("empty dataframe transform")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}});
  df.push_back({.output = 1.0, .input = {2.0}});

  standard_scaler scaler;
  scaler.fit(df);

  dataframe empty_df;
  scaler.transform(empty_df);
  CHECK(empty_df.empty());
}

TEST_CASE("re-fitting")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  dataframe df1;
  df1.set_schema({{"y", d_double}, {"x0", d_double}});
  df1.push_back({.output = 1.0, .input = {10.0}});
  df1.push_back({.output = 2.0, .input = {20.0}});
  df1.push_back({.output = 3.0, .input = {30.0}});

  standard_scaler scaler;
  scaler.fit(df1);
  CHECK(scaler.is_fitted());
  CHECK(scaler.is_valid());

  dataframe df2;
  df2.set_schema({{"y", d_double}, {"x0", d_double}, {"x1", d_double}});
  df2.push_back({.output = 10.0, .input = {100.0, 1.0}});
  df2.push_back({.output = 20.0, .input = {200.0, 2.0}});
  df2.push_back({.output = 30.0, .input = {300.0, 3.0}});

  scaler.fit(df2);
  CHECK(scaler.is_fitted());
  CHECK(scaler.is_valid());

  scaler.transform(df2);
  const auto it(df2.begin());

  CHECK(std::get<D_DOUBLE>(it[0].input[0]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[0]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[0]) == doctest::Approx(1.0));

  CHECK(std::get<D_DOUBLE>(it[0].input[1]) == doctest::Approx(-1.0));
  CHECK(std::get<D_DOUBLE>(it[1].input[1]) == doctest::Approx(0.0));
  CHECK(std::get<D_DOUBLE>(it[2].input[1]) == doctest::Approx(1.0));
}

TEST_CASE("wine dataset standardization")
{
  using namespace ultra;
  using src::dataframe;
  using src::standard_scaler;

  const auto p(dataframe::params().output(dataframe::params::index::back)
                                  .strong_data_typing());
  dataframe df(debug::wine_table, p);
  const auto original_df(df);

  standard_scaler scaler;
  scaler.fit(df);
  scaler.transform(df);

  const auto vars(df.variables());
  for (unsigned j(0); j < vars; ++j)
  {
    double sum(0.0);
    for (const auto &ex : df)
      sum += std::get<D_DOUBLE>(ex.input[j]);
    const double mean(sum / static_cast<double>(df.size()));
    CHECK(mean == doctest::Approx(0.0).epsilon(1e-6));

    double sq_diff_sum(0.0);
    for (const auto &ex : df)
    {
      const double diff(std::get<D_DOUBLE>(ex.input[j]) - mean);
      sq_diff_sum += diff * diff;
    }
    const double sample_std(std::sqrt(sq_diff_sum
                                      / static_cast<double>(df.size() - 1)));
    CHECK(sample_std == doctest::Approx(1.0).epsilon(1e-6));
  }

  // Target in wine is int (quality), so it should remain untouched
  const auto it_df(df.begin());
  const auto it_orig(original_df.begin());
  for (std::size_t i(0); i < df.size(); ++i)
    CHECK(std::get<D_INT>(it_df[i].output) == std::get<D_INT>(it_orig[i].output));
}

}  // TEST_SUITE("src::standard_scaler")
