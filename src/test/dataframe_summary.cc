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

#include "kernel/gp/src/dataframe_summary.h"
#include "kernel/gp/src/dataframe.h"
#include "test/debug_datasets.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("src::dataframe_summary")
{

TEST_CASE("empty dataframe")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;

  const dataframe df;
  const dataframe_summary summary(df);

  CHECK(summary.rows() == 0);
  CHECK(summary.columns() == 0);
  CHECK(!summary.range("unknown").has_value());
  CHECK_THROWS_AS(static_cast<void>(summary["unknown"]), std::out_of_range);

  std::ostringstream oss;
  oss << summary;
  CHECK(oss.str() == "[empty dataframe_summary]\n");
}

TEST_CASE("numerical dataset")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::numeric_stats;

  dataframe df;
  df.set_schema({{"y", d_double}, {"x0", d_double}, {"x1", d_double}});
  df.push_back({.output = 10.0, .input = {1.0, 100.0}});
  df.push_back({.output = 20.0, .input = {2.0, 200.0}});
  df.push_back({.output = 30.0, .input = {3.0, 300.0}});

  const dataframe_summary summary(df);

  CHECK(summary.rows() == 3);
  CHECK(summary.columns() == 3);

  SUBCASE("element access by index and name")
  {
    CHECK(summary[0].name == "y");
    CHECK(summary[0].domain == d_double);
    CHECK(summary[1].name == "x0");
    CHECK(summary[2].name == "x1");

    CHECK(summary["y"].name == "y");
    CHECK(summary["x0"].name == "x0");
    CHECK(summary["x1"].name == "x1");

    CHECK_THROWS_AS(static_cast<void>(summary["non_existent"]),
                    std::out_of_range);
  }

  SUBCASE("numerical statistics values")
  {
    REQUIRE(std::holds_alternative<numeric_stats>(summary[0].stats));
    const auto &stats_y(std::get<numeric_stats>(summary[0].stats));

    CHECK(stats_y.dist.size() == 3);
    CHECK(stats_y.dist.mean() == doctest::Approx(20.0));
    CHECK(stats_y.dist.min() == doctest::Approx(10.0));
    CHECK(stats_y.dist.max() == doctest::Approx(30.0));
    CHECK(stats_y.dist.standard_deviation()
          == doctest::Approx(std::sqrt(200.0 / 3.0)));

    REQUIRE(std::holds_alternative<numeric_stats>(summary[1].stats));
    const auto &stats_x0(std::get<numeric_stats>(summary[1].stats));
    CHECK(stats_x0.dist.mean() == doctest::Approx(2.0));
    CHECK(stats_x0.dist.min() == doctest::Approx(1.0));
    CHECK(stats_x0.dist.max() == doctest::Approx(3.0));
    CHECK(stats_x0.dist.standard_deviation()
          == doctest::Approx(std::sqrt(2.0 / 3.0)));
  }

  SUBCASE("range lookups")
  {
    const auto r0(summary.range(0));
    REQUIRE(r0.has_value());
    CHECK(r0->first == doctest::Approx(10.0));
    CHECK(r0->second == doctest::Approx(30.0));

    const auto r_name(summary.range("y"));
    REQUIRE(r_name.has_value());
    CHECK(r_name->first == doctest::Approx(10.0));
    CHECK(r_name->second == doctest::Approx(30.0));

    const auto r1(summary.range("x0"));
    REQUIRE(r1.has_value());
    CHECK(r1->first == doctest::Approx(1.0));
    CHECK(r1->second == doctest::Approx(3.0));

    CHECK(!summary.range("non_existent").has_value());
  }
}

TEST_CASE("categorical dataset")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::categorical_stats;

  dataframe df;
  df.set_schema({{"y", d_int}, {"color", d_string}});
  df.push_back({.output = 0, .input = {std::string("red")}});
  df.push_back({.output = 1, .input = {std::string("blue")}});
  df.push_back({.output = 0, .input = {std::string("red")}});
  df.push_back({.output = 1, .input = {std::string("green")}});
  df.push_back({.output = 0, .input = {std::string("red")}});

  const dataframe_summary summary(df);

  CHECK(summary.rows() == 5);
  CHECK(summary.columns() == 2);

  REQUIRE(std::holds_alternative<categorical_stats>(summary[1].stats));
  const auto &cat(std::get<categorical_stats>(summary[1].stats));

  CHECK(cat.count == 5);
  CHECK(cat.unique == 3);
  CHECK(cat.mode == "red");
  CHECK(cat.mode_count == 3);

  CHECK(!summary.range(1).has_value());
  CHECK(!summary.range("color").has_value());
}

TEST_CASE("categorical whitespace trimming")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::categorical_stats;

  dataframe df;
  df.set_schema({{"y", d_int}, {"label", d_string}});
  df.push_back({.output = 0, .input = {std::string("  alpha ")}});
  df.push_back({.output = 1, .input = {std::string("alpha")}});
  df.push_back({.output = 0, .input = {std::string("alpha\t")}});

  const dataframe_summary summary(df);

  REQUIRE(std::holds_alternative<categorical_stats>(summary[1].stats));
  const auto &cat(std::get<categorical_stats>(summary[1].stats));

  CHECK(cat.count == 3);
  CHECK(cat.unique == 1);
  CHECK(cat.mode == "alpha");
  CHECK(cat.mode_count == 3);
}

TEST_CASE("unsupervised dataset with d_void target")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::void_stats;
  using src::numeric_stats;

  dataframe::params p;
  p.no_output();

  const std::vector<src::record_t> matrix =
  {
    {"f0", "f1"},
    {1.0, 10.0},
    {2.0, 20.0},
    {3.0, 30.0}
  };

  const dataframe df(matrix, p);
  const dataframe_summary summary(df);

  CHECK(summary.rows() == 3);
  CHECK(summary.columns() == 3);

  // Column 0 is the dummy target with d_void.
  CHECK(summary[0].domain == d_void);
  CHECK(std::holds_alternative<void_stats>(summary[0].stats));
  CHECK(!summary.range(0).has_value());

  // Columns 1 and 2 are feature columns.
  REQUIRE(std::holds_alternative<numeric_stats>(summary[1].stats));
  const auto &f0(std::get<numeric_stats>(summary[1].stats));
  CHECK(f0.dist.size() == 3);
  CHECK(f0.dist.mean() == doctest::Approx(2.0));

  REQUIRE(std::holds_alternative<numeric_stats>(summary[2].stats));
  const auto &f1(std::get<numeric_stats>(summary[2].stats));
  CHECK(f1.dist.size() == 3);
  CHECK(f1.dist.mean() == doctest::Approx(20.0));
}

TEST_CASE("edge cases: single row and constant values")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::numeric_stats;

  SUBCASE("single row")
  {
    dataframe df;
    df.set_schema({{"y", d_double}, {"x", d_double}});
    df.push_back({.output = 42.0, .input = {3.14}});

    const dataframe_summary summary(df);
    CHECK(summary.rows() == 1);

    const auto r(summary.range(0));
    REQUIRE(r.has_value());
    CHECK(r->first == doctest::Approx(42.0));
    CHECK(r->second == doctest::Approx(42.0));
  }

  SUBCASE("constant values")
  {
    dataframe df;
    df.set_schema({{"y", d_double}, {"x", d_double}});
    df.push_back({.output = 5.0, .input = {7.0}});
    df.push_back({.output = 5.0, .input = {7.0}});
    df.push_back({.output = 5.0, .input = {7.0}});

    const dataframe_summary summary(df);

    const auto &stat_x(std::get<numeric_stats>(summary[1].stats));
    CHECK(stat_x.dist.min() == doctest::Approx(7.0));
    CHECK(stat_x.dist.max() == doctest::Approx(7.0));
    CHECK(stat_x.dist.mean() == doctest::Approx(7.0));
    CHECK(stat_x.dist.standard_deviation() == doctest::Approx(0.0));
  }
}

TEST_CASE("pretty-printing operator<<")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;

  dataframe df;
  df.set_schema({{"age", d_double}, {"city", d_string}});
  df.push_back({.output = 25.0, .input = {std::string("Rome")}});
  df.push_back({.output = 30.0, .input = {std::string("Milan")}});
  df.push_back({.output = 35.0, .input = {std::string("Rome")}});

  const dataframe_summary summary(df);

  std::ostringstream oss;
  oss << summary;
  const std::string out(oss.str());

  // Check headers and metrics are present.
  CHECK(out.find("age") != std::string::npos);
  CHECK(out.find("city") != std::string::npos);
  CHECK(out.find("count") != std::string::npos);
  CHECK(out.find("mean") != std::string::npos);
  CHECK(out.find("unique") != std::string::npos);
  CHECK(out.find("top") != std::string::npos);
  CHECK(out.find("freq") != std::string::npos);
  CHECK(out.find("Rome") != std::string::npos);
  CHECK(out.find("NaN") != std::string::npos);
}

TEST_CASE("abalone dataset integration")
{
  using namespace ultra;
  using src::dataframe;
  using src::dataframe_summary;
  using src::categorical_stats;
  using src::numeric_stats;

  const auto p(dataframe::params().output(dataframe::params::index::back));
  const dataframe df(debug::abalone_table, p);
  const dataframe_summary summary(df);

  CHECK(summary.rows() == debug::ABALONE_COUNT);
  CHECK(summary.columns() == 9);

  // Sex column is categorical.
  const auto &sex_col(summary["sex"]);
  CHECK(sex_col.domain == d_string);
  REQUIRE(std::holds_alternative<categorical_stats>(sex_col.stats));

  const auto &sex_stats(std::get<categorical_stats>(sex_col.stats));
  CHECK(sex_stats.count == debug::ABALONE_COUNT);
  CHECK(sex_stats.unique == 3);  // M, F, I

  // Numerical columns have valid ranges.
  const auto length_range(summary.range("length"));
  REQUIRE(length_range.has_value());
  CHECK(length_range->first > 0.0);
  CHECK(length_range->second > length_range->first);

  const auto rings_range(summary.range("rings"));
  REQUIRE(rings_range.has_value());
  CHECK(rings_range->first >= 7.0);
  CHECK(rings_range->second <= 20.0);
}

TEST_CASE("classification dataset")
{
  using namespace ultra;
  using src::categorical_stats;
  using src::dataframe;
  using src::dataframe_summary;

  SUBCASE("string class labels")
  {
    const auto p(dataframe::params().output(dataframe::params::index::back));
    const dataframe df(debug::iris_table, p);
    const dataframe_summary summary(df);

    CHECK(summary.rows() == debug::IRIS_COUNT);
    CHECK(summary.columns() == 5);

    REQUIRE(std::holds_alternative<categorical_stats>(summary[0].stats));
    const auto &cls(std::get<categorical_stats>(summary[0].stats));

    CHECK(cls.count == debug::IRIS_COUNT);
    CHECK(cls.unique == 3);
    CHECK(cls.mode == "Iris-virginica");
    CHECK(cls.mode_count == 4);

    CHECK(!summary.range(0).has_value());
    CHECK(!summary.range("class").has_value());
    CHECK(summary.range("sepal length").has_value());
  }

  SUBCASE("integer class labels without class_name mapping")
  {
    dataframe df;
    df.set_schema({{"y", d_string}, {"x", d_double}});
    df.push_back({.output = 0, .input = {1.0}});
    df.push_back({.output = 1, .input = {2.0}});
    df.push_back({.output = 0, .input = {3.0}});

    const dataframe_summary summary(df);

    REQUIRE(std::holds_alternative<categorical_stats>(summary[0].stats));
    const auto &cls(std::get<categorical_stats>(summary[0].stats));

    CHECK(cls.count == 3);
    CHECK(cls.unique == 2);
    CHECK(cls.mode == "0");
    CHECK(cls.mode_count == 2);

    CHECK(!summary.range(0).has_value());
    CHECK(summary.range("x").has_value());
  }
}

TEST_CASE("dataset with middle d_void feature column")
{
  using namespace ultra;
  using src::categorical_stats;
  using src::dataframe;
  using src::dataframe_summary;
  using src::numeric_stats;
  using src::void_stats;

  std::istringstream input("target,empty,x,y\n"
                           "1.0,,2.5,red\n"
                           "2.0,,4.5,blue\n"
                           "3.0,,4.5,red\n");
  const dataframe df(input, dataframe::params().header());
  const dataframe_summary summary(df);

  CHECK(summary.rows() == 3);
  CHECK(summary.columns() == 4);

  CHECK(summary[1].domain == d_void);
  CHECK(std::holds_alternative<void_stats>(summary[1].stats));
  CHECK(!summary.range(1).has_value());

  REQUIRE(std::holds_alternative<numeric_stats>(summary[2].stats));
  const auto &x_stats(std::get<numeric_stats>(summary[2].stats));
  CHECK(x_stats.dist.size() == 3);
  CHECK(x_stats.dist.min() == doctest::Approx(2.5));
  CHECK(x_stats.dist.max() == doctest::Approx(4.5));

  REQUIRE(std::holds_alternative<categorical_stats>(summary[3].stats));
  const auto &y_stats(std::get<categorical_stats>(summary[3].stats));
  CHECK(y_stats.count == 3);
  CHECK(y_stats.unique == 2);
  CHECK(y_stats.mode == "red");
  CHECK(y_stats.mode_count == 2);
}

}  // TEST_SUITE("src::dataframe_summary")
