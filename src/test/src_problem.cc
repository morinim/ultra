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

#include <sstream>

#include "kernel/gp/src/problem.h"
#include "kernel/gp/primitive/real.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SRC::PROBLEM")
{

TEST_CASE("Base")
{
  using namespace ultra;

  src::problem p;
  CHECK(p.is_valid());

  CHECK(p.sset.categories() == 0);
  p.insert<real::add>();
  CHECK(p.sset.categories() == 1);
}

TEST_CASE("setup_terminals")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream wine(R"(
    fixed acidity,volatile acidity,citric acid,residual sugar,chlorides,free sulfur dioxide,total sulfur dioxide,density,pH,sulphates,alcohol,quality
     7.4,0.7, 0,   1.9,0.076,11, 34,0.9978,3.51,0.56, 9.4,5
     7.8,0.88,0,   2.6,0.098,25, 67,0.9968,3.2, 0.68, 9.8,5
     7.8,0.76,0.04,2.3,0.092,15, 54,0.997, 3.26,0.65, 9.8,5
    11.2,0.28,0.56,1.9,0.075,17, 60,0.998, 3.16,0.58, 9.8,6
     7.4,0.7, 0,   1.9,0.076,11, 34,0.9978,3.51,0.56, 9.4,5
     7.4,0.66,0,   1.8,0.075,13, 40,0.9978,3.51,0.56, 9.4,5
     7.9,0.6, 0.06,1.6,0.069,15, 59,0.9964,3.3, 0.46, 9.4,5
     7.3,0.65,0,   1.2,0.065,15, 21,0.9946,3.39,0.47,10,  7
     7.8,0.58,0.02,2,  0.073, 9, 18,0.9968,3.36,0.57, 9.5,7
     7.5,0.5, 0.36,6.1,0.071,17,102,0.9978,3.35,0.8, 10.5,5)");

  std::istringstream iris(R"(
    sepal length,sepal width,petal length,petal width,class
    5.1,3.5,1.4,0.2,Iris-setosa
    4.9,3,1.4,0.2,Iris-setosa
    4.7,3.2,1.3,0.2,Iris-setosa
    7,3.2,4.7,1.4,Iris-versicolor
    6.4,3.2,4.5,1.5,Iris-versicolor
    6.9,3.1,4.9,1.5,Iris-versicolor
    6.3,2.5,5,1.9,Iris-virginica
    6.5,3,5.2,2,Iris-virginica
    6.2,3.4,5.4,2.3,Iris-virginica
    5.9,3,5.1,1.8,Iris-virginica)");

  SUBCASE("Weak typing - Symbolic regression")
  {
    src::problem p(wine);
    p.setup_symbols();

    CHECK(p.is_valid());

    CHECK(p.categories() == 1);
    CHECK(p.classes() == 0);
    CHECK(p.variables() == 11);

    CHECK(!p.classification());

    CHECK(p.data().size());
    CHECK(p.data(src::dataset_t::validation).empty());
  }

  SUBCASE("Strong typing - Symbolic regression")
  {
    src::dataframe::params params;
    params.data_typing = src::typing::strong;
    params.output_index = 11;

    src::problem p(wine, params);
    p.setup_symbols();

    CHECK(p.is_valid());

    CHECK(p.categories() == 12);
    CHECK(p.classes() == 0);
    CHECK(p.variables() == 11);

    CHECK(!p.classification());

    CHECK(p.data().size());
    CHECK(p.data(src::dataset_t::validation).empty());
  }

  SUBCASE("Weak typing - Classification")
  {
    src::dataframe::params params;
    params.output_index = 4;

    src::problem p(iris, params);
    p.setup_symbols();

    CHECK(p.is_valid());

    CHECK(p.categories() == 1);
    CHECK(p.classes() == 3);
    CHECK(p.variables() == 4);

    CHECK(p.classification());

    CHECK(p.data().size());
    CHECK(p.data(src::dataset_t::validation).empty());
  }

  SUBCASE("Strong typing - Classification")
  {
    src::dataframe::params params;
    params.data_typing = src::typing::strong;
    params.output_index = 4;

    src::problem p(iris, params);
    p.setup_symbols();
    CHECK(p.sset.enough_terminals());

    CHECK(p.categories() == 5);
    CHECK(p.classes() == 3);
    CHECK(p.variables() == 4);

    CHECK(p.classification());

    CHECK(p.data().size());
    CHECK(p.data(src::dataset_t::validation).empty());
  }
}

}  // TEST_SUITE("SRC::PROBLEM")
