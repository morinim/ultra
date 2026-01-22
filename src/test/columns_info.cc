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

#include "kernel/gp/src/columns_info.h"
#include "kernel/gp/src/dataframe.h"

#include <sstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("COLUMNS_INFO")
{

struct fixture_ci
{
  ultra::src::dataframe d {};
  ultra::src::dataframe::params p {};

  const ultra::src::columns_info &cs {d.columns};
};

TEST_CASE_FIXTURE(fixture_ci, "wine categories weak")
{
  using namespace ultra;

  SUBCASE("stringstream")
  {
    std::istringstream wine(debug::wine);
    CHECK(d.read(wine) == debug::WINE_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::wine_table) == debug::WINE_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "fixed acidity");
  CHECK(cs[1].name() == "volatile acidity");
  CHECK(cs[2].name() == "citric acid");
  CHECK(cs[3].name() == "residual sugar");
  CHECK(cs[4].name() == "chlorides");
  CHECK(cs[5].name() == "free sulfur dioxide");
  CHECK(cs[6].name() == "total sulfur dioxide");
  CHECK(cs[7].name() == "density");
  CHECK(cs[8].name() == "pH");
  CHECK(cs[9].name() == "sulphates");
  CHECK(cs[10].name() == "alcohol");
  CHECK(cs[11].name() == "quality");

  CHECK(std::all_of(cs.begin(), std::prev(cs.end()),
                    [](const auto &c) { return c.domain() == d_double; }));
  CHECK(cs.back().domain() == d_int);

  CHECK(cs.domain_of_category(0) == d_double);
  CHECK(cs.domain_of_category(1) == d_int);

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0, 1});

  CHECK(cs.task() == src::task_t::regression);
}

TEST_CASE("wine categories strong")
{
  using namespace ultra;
  using src::dataframe;

  dataframe d;

  SUBCASE("stringstream")
  {
    std::istringstream wine(debug::wine);
    dataframe ds(wine, dataframe::params().strong_data_typing());
    d = ds;
  }

  SUBCASE("table")
  {
    dataframe dt(debug::wine_table, dataframe::params().strong_data_typing());
    d = dt;
  }

  CHECK(d.is_valid());
  CHECK(d.size() == debug::WINE_COUNT);

  const auto &cs(d.columns);
  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "fixed acidity");
  CHECK(cs[0].category() == 0);
  CHECK(cs[1].name() == "volatile acidity");
  CHECK(cs[1].category() == 1);
  CHECK(cs[2].name() == "citric acid");
  CHECK(cs[2].category() == 2);
  CHECK(cs[3].name() == "residual sugar");
  CHECK(cs[3].category() == 3);
  CHECK(cs[4].name() == "chlorides");
  CHECK(cs[4].category() == 4);
  CHECK(cs[5].name() == "free sulfur dioxide");
  CHECK(cs[5].category() == 5);
  CHECK(cs[6].name() == "total sulfur dioxide");
  CHECK(cs[6].category() == 6);
  CHECK(cs[7].name() == "density");
  CHECK(cs[7].category() == 7);
  CHECK(cs[8].name() == "pH");
  CHECK(cs[8].category() == 8);
  CHECK(cs[9].name() == "sulphates");
  CHECK(cs[9].category() == 9);
  CHECK(cs[10].name() == "alcohol");
  CHECK(cs[10].category() == 10);
  CHECK(cs[11].name() == "quality");
  CHECK(cs[11].category() == 11);

  CHECK(cs.used_categories()
        == std::set<symbol::category_t>{0,1,2,3,4,5,6,7,8,9,10,11});

  CHECK(std::ranges::all_of(
          cs.begin(), std::prev(cs.end()),
          [](auto c) { return c.domain() == d_double; }));
  CHECK(cs.back().domain() == d_int);

  CHECK(std::ranges::all_of(
          cs.used_categories(),
          [&cs] (const auto &c)
          {
            return cs.domain_of_category(c) == (c==11 ? d_int : d_double);
          }));

  CHECK(cs.task() == src::task_t::regression);
}

TEST_CASE_FIXTURE(fixture_ci, "abalone categories weak")
{
  using namespace ultra;

  p.output_index = 8;

  SUBCASE("stringstream")
  {
    std::istringstream abalone(debug::abalone);
    CHECK(d.read(abalone, p) == debug::ABALONE_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::abalone_table, p) == debug::ABALONE_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "rings");
  CHECK(cs[0].domain() == d_int);
  CHECK(cs[0].category() == 0);
  CHECK(cs[1].name() == "sex");
  CHECK(cs[1].domain() == d_string);
  CHECK(cs[1].category() == 1);
  CHECK(cs[2].name() == "length");
  CHECK(cs[2].domain() == d_double);
  CHECK(cs[2].category() == 2);
  CHECK(cs[3].name() == "diameter");
  CHECK(cs[3].domain() == d_double);
  CHECK(cs[3].category() == 2);
  CHECK(cs[4].name() == "height");
  CHECK(cs[4].domain() == d_double);
  CHECK(cs[4].category() == 2);
  CHECK(cs[5].name() == "whole weight");
  CHECK(cs[5].domain() == d_double);
  CHECK(cs[5].category() == 2);
  CHECK(cs[6].name() == "shucked weight");
  CHECK(cs[6].domain() == d_double);
  CHECK(cs[6].category() == 2);
  CHECK(cs[7].name() == "viscera weight");
  CHECK(cs[7].domain() == d_double);
  CHECK(cs[7].category() == 2);
  CHECK(cs[8].name() == "shell weight");
  CHECK(cs[8].domain() == d_double);
  CHECK(cs[8].category() == 2);

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0,1,2});

  CHECK(cs.domain_of_category(0) == d_int);
  CHECK(cs.domain_of_category(1) == d_string);
  CHECK(cs.domain_of_category(2) == d_double);

  CHECK(cs.task() == src::task_t::regression);
}

TEST_CASE_FIXTURE(fixture_ci, "abalone categories strong")
{
  using namespace ultra;

  SUBCASE("stringstream")
  {
    std::istringstream abalone(debug::abalone);

    CHECK(d.read(abalone,
                 src::dataframe::params().strong_data_typing().output(8))
          == debug::ABALONE_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::abalone_table,
                 src::dataframe::params().strong_data_typing().output(8))
          == debug::ABALONE_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "rings");
  CHECK(cs[0].domain() == d_int);
  CHECK(cs[0].category() == 0);
  CHECK(cs[1].name() == "sex");
  CHECK(cs[1].domain() == d_string);
  CHECK(cs[1].category() == 1);
  CHECK(cs[2].name() == "length");
  CHECK(cs[2].domain() == d_double);
  CHECK(cs[2].category() == 2);
  CHECK(cs[3].name() == "diameter");
  CHECK(cs[3].domain() == d_double);
  CHECK(cs[3].category() == 3);
  CHECK(cs[4].name() == "height");
  CHECK(cs[4].domain() == d_double);
  CHECK(cs[4].category() == 4);
  CHECK(cs[5].name() == "whole weight");
  CHECK(cs[5].domain() == d_double);
  CHECK(cs[5].category() == 5);
  CHECK(cs[6].name() == "shucked weight");
  CHECK(cs[6].domain() == d_double);
  CHECK(cs[6].category() == 6);
  CHECK(cs[7].name() == "viscera weight");
  CHECK(cs[7].domain() == d_double);
  CHECK(cs[7].category() == 7);
  CHECK(cs[8].name() == "shell weight");
  CHECK(cs[8].domain() == d_double);
  CHECK(cs[8].category() == 8);

  const auto used_categories(cs.used_categories());

  CHECK(used_categories == std::set<symbol::category_t>{0,1,2,3,4,5,6,7,8});

  for (auto c : used_categories)
    switch (c)
    {
    case 0:
      CHECK(cs.domain_of_category(c) == d_int);
      break;
    case 1:
      CHECK(cs.domain_of_category(c) == d_string);
      break;
    default:
      CHECK(cs.domain_of_category(c) == d_double);
    }

  CHECK(cs.task() == src::task_t::regression);
}

TEST_CASE_FIXTURE(fixture_ci, "ecoli categories")
{
  using namespace ultra;

  p.output_index = std::nullopt;

  SUBCASE("stringstream")
  {
    std::istringstream ecoli(debug::ecoli);
    CHECK(d.read(ecoli, p) == debug::ECOLI_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::ecoli_table, p) == debug::ECOLI_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "");
  CHECK(cs[0].domain() == d_void);
  CHECK(cs[0].category() == symbol::undefined_category);
  CHECK(cs[1].name() == "sequence name");
  CHECK(cs[1].domain() == d_string);
  CHECK(cs[1].category() == 0);
  CHECK(cs[2].name() == "mcg");
  CHECK(cs[2].domain() == d_double);
  CHECK(cs[2].category() == 1);
  CHECK(cs[3].name() == "gvh");
  CHECK(cs[3].domain() == d_double);
  CHECK(cs[3].category() == 1);
  CHECK(cs[4].name() == "lip");
  CHECK(cs[4].domain() == d_double);
  CHECK(cs[4].category() == 1);
  CHECK(cs[5].name() == "chg");
  CHECK(cs[5].domain() == d_double);
  CHECK(cs[5].category() == 1);
  CHECK(cs[6].name() == "aac");
  CHECK(cs[6].domain() == d_double);
  CHECK(cs[6].category() == 1);
  CHECK(cs[7].name() == "alm1");
  CHECK(cs[7].domain() == d_double);
  CHECK(cs[7].category() == 1);
  CHECK(cs[8].name() == "alm2");
  CHECK(cs[8].domain() == d_double);
  CHECK(cs[8].category() == 1);
  CHECK(cs[9].name() == "localization");
  CHECK(cs[9].domain() == d_string);
  CHECK(cs[9].category() == 2);

  CHECK(cs.used_categories()
        == std::set<symbol::category_t>{0, 1, 2, symbol::undefined_category});

  CHECK(cs.domain_of_category(symbol::undefined_category) == d_void);
  CHECK(cs.domain_of_category(0) == d_string);
  CHECK(cs.domain_of_category(1) == d_double);
  CHECK(cs.domain_of_category(2) == d_string);

  CHECK(cs.task() == src::task_t::unsupervised);
}

TEST_CASE_FIXTURE(fixture_ci, "ecoli categories strong")
{
  using namespace ultra;

  p.output_index = std::nullopt;
  p.data_typing = src::typing::strong;

  SUBCASE("stringstream")
  {
    std::istringstream ecoli(debug::ecoli);
    CHECK(d.read(ecoli, p) == debug::ECOLI_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::ecoli_table, p) == debug::ECOLI_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "");
  CHECK(cs[0].domain() == d_void);
  CHECK(cs[0].category() == symbol::undefined_category);
  CHECK(cs[1].name() == "sequence name");
  CHECK(cs[1].domain() == d_string);
  CHECK(cs[1].category() == 0);
  CHECK(cs[2].name() == "mcg");
  CHECK(cs[2].domain() == d_double);
  CHECK(cs[2].category() == 1);
  CHECK(cs[3].name() == "gvh");
  CHECK(cs[3].domain() == d_double);
  CHECK(cs[3].category() == 2);
  CHECK(cs[4].name() == "lip");
  CHECK(cs[4].domain() == d_double);
  CHECK(cs[4].category() == 3);
  CHECK(cs[5].name() == "chg");
  CHECK(cs[5].domain() == d_double);
  CHECK(cs[5].category() == 4);
  CHECK(cs[6].name() == "aac");
  CHECK(cs[6].domain() == d_double);
  CHECK(cs[6].category() == 5);
  CHECK(cs[7].name() == "alm1");
  CHECK(cs[7].domain() == d_double);
  CHECK(cs[7].category() == 6);
  CHECK(cs[8].name() == "alm2");
  CHECK(cs[8].domain() == d_double);
  CHECK(cs[8].category() == 7);
  CHECK(cs[9].name() == "localization");
  CHECK(cs[9].domain() == d_string);
  CHECK(cs[9].category() == 8);

  CHECK(cs.used_categories()
        == std::set<symbol::category_t>{0, 1, 2, 3, 4, 5, 6, 7, 8,
                                        symbol::undefined_category});

  CHECK(cs.domain_of_category(symbol::undefined_category) == d_void);
  CHECK(cs.domain_of_category(0) == d_string);
  CHECK(cs.domain_of_category(8) == d_string);

  for (symbol::category_t c(1); c <= 7; ++c)
    CHECK(cs.domain_of_category(c) == d_double);

  CHECK(cs.task() == src::task_t::unsupervised);
}

TEST_CASE_FIXTURE(fixture_ci, "load_csv classification")
{
  using namespace ultra;

  p.output_index = 4;

  SUBCASE("stringstream")
  {
    std::istringstream iris(debug::iris);
    CHECK(d.read(iris, p) == debug::IRIS_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::iris_table, p) == debug::IRIS_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "class");
  CHECK(cs[1].name() == "sepal length");
  CHECK(cs[2].name() == "sepal width");
  CHECK(cs[3].name() == "petal length");
  CHECK(cs[4].name() == "petal width");

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0});
  CHECK(cs.domain_of_category(0) == d_double);

  CHECK(cs[0].domain() == d_int);
  CHECK(std::all_of(std::next(cs.begin()), cs.end(),
                    [](auto c) { return c.domain() == d_double; }));

  CHECK(cs.task() == src::task_t::classification);
}

TEST_CASE_FIXTURE(fixture_ci, "load_csv classification strong")
{
  using namespace ultra;

  p.output_index = 4;
  p.data_typing = src::typing::strong;

  SUBCASE("stringstream")
  {
    std::istringstream iris(debug::iris);
    CHECK(d.read(iris, p) == debug::IRIS_COUNT);
  }

  SUBCASE("table")
  {
    CHECK(d.read(debug::iris_table, p) == debug::IRIS_COUNT);
  }

  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "class");
  CHECK(cs[0].category() == 0);
  CHECK(cs[1].name() == "sepal length");
  CHECK(cs[1].category() == 1);
  CHECK(cs[2].name() == "sepal width");
  CHECK(cs[2].category() == 2);
  CHECK(cs[3].name() == "petal length");
  CHECK(cs[3].category() == 3);
  CHECK(cs[4].name() == "petal width");
  CHECK(cs[4].category() == 4);

  const auto used_categories(cs.used_categories());

  CHECK(used_categories == std::set<symbol::category_t>{0, 1, 2, 3, 4});

  for (auto c : used_categories)
    CHECK(cs.domain_of_category(c) == d_double);

  CHECK(cs[0].domain() == d_int);
  CHECK(std::all_of(std::next(cs.begin()), cs.end(),
                    [](auto c) { return c.domain() == d_double; }));

  CHECK(cs.task() == src::task_t::classification);
}

}  // TEST_SUITE("COLUMNS_INFO")
