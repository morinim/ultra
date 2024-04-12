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

#include "kernel/gp/src/columns_info.h"
#include "kernel/gp/src/dataframe.h"

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

  CHECK(d.read_csv(wine) == 10);
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

  CHECK(std::ranges::all_of(cs, [](auto c) { return c.domain() == d_double; }));

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0});
}

TEST_CASE("wine categories strong")
{
  using namespace ultra;
  using src::dataframe;

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

  dataframe d(wine, dataframe::params().strong_data_typing());
  CHECK(d.is_valid());

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

  CHECK(std::ranges::all_of(cs, [](auto c) { return c.domain() == d_double; }));
}

TEST_CASE_FIXTURE(fixture_ci, "abalone categories weak")
{
  using namespace ultra;

  std::istringstream abalone(R"(
    sex,length,diameter,height,whole weight,shucked weight,viscera weight,shell weight,rings
    M,0.455,0.365,0.095,0.514,0.2245,0.101,0.15,15
    M,0.35,0.265,0.09,0.2255,0.0995,0.0485,0.07,7
    F,0.53,0.42,0.135,0.677,0.2565,0.1415,0.21,9
    M,0.44,0.365,0.125,0.516,0.2155,0.114,0.155,10
    I,0.33,0.255,0.08,0.205,0.0895,0.0395,0.055,7
    I,0.425,0.3,0.095,0.3515,0.141,0.0775,0.12,8
    F,0.53,0.415,0.15,0.7775,0.237,0.1415,0.33,20
    F,0.545,0.425,0.125,0.768,0.294,0.1495,0.26,16
    M,0.475,0.37,0.125,0.5095,0.2165,0.1125,0.165,9
    F,0.55,0.44,0.15,0.8945,0.3145,0.151,0.32,19)");

  p.output_index = 8;
  CHECK(d.read_csv(abalone, p) == 10);
  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "rings");
  CHECK(cs[0].domain() == d_double);
  CHECK(cs[0].category() == 0);
  CHECK(cs[1].name() == "sex");
  CHECK(cs[1].domain() == d_string);
  CHECK(cs[1].category() == 1);
  CHECK(cs[2].name() == "length");
  CHECK(cs[2].domain() == d_double);
  CHECK(cs[2].category() == 0);
  CHECK(cs[3].name() == "diameter");
  CHECK(cs[3].domain() == d_double);
  CHECK(cs[3].category() == 0);
  CHECK(cs[4].name() == "height");
  CHECK(cs[4].domain() == d_double);
  CHECK(cs[4].category() == 0);
  CHECK(cs[5].name() == "whole weight");
  CHECK(cs[5].domain() == d_double);
  CHECK(cs[5].category() == 0);
  CHECK(cs[6].name() == "shucked weight");
  CHECK(cs[6].domain() == d_double);
  CHECK(cs[6].category() == 0);
  CHECK(cs[7].name() == "viscera weight");
  CHECK(cs[7].domain() == d_double);
  CHECK(cs[7].category() == 0);
  CHECK(cs[8].name() == "shell weight");
  CHECK(cs[8].domain() == d_double);
  CHECK(cs[8].category() == 0);

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0,1});
}

TEST_CASE_FIXTURE(fixture_ci, "abalone categories strong")
{
  using namespace ultra;

  std::istringstream abalone(R"(
    sex,length,diameter,height,whole weight,shucked weight,viscera weight,shell weight,rings
    M,0.455,0.365,0.095,0.514,0.2245,0.101,0.15,15
    M,0.35,0.265,0.09,0.2255,0.0995,0.0485,0.07,7
    F,0.53,0.42,0.135,0.677,0.2565,0.1415,0.21,9
    M,0.44,0.365,0.125,0.516,0.2155,0.114,0.155,10
    I,0.33,0.255,0.08,0.205,0.0895,0.0395,0.055,7
    I,0.425,0.3,0.095,0.3515,0.141,0.0775,0.12,8
    F,0.53,0.415,0.15,0.7775,0.237,0.1415,0.33,20
    F,0.545,0.425,0.125,0.768,0.294,0.1495,0.26,16
    M,0.475,0.37,0.125,0.5095,0.2165,0.1125,0.165,9
    F,0.55,0.44,0.15,0.8945,0.3145,0.151,0.32,19)");

  CHECK(d.read_csv(abalone,
                   src::dataframe::params().strong_data_typing().output(8))
        == 10);
  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "rings");
  CHECK(cs[0].domain() == d_double);
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

  CHECK(cs.used_categories()
        == std::set<symbol::category_t>{0,1,2,3,4,5,6,7,8});
}

TEST_CASE_FIXTURE(fixture_ci, "ecoli categories")
{
  using namespace ultra;

  std::istringstream ecoli(R"(
    sequence name, mcg,  gvh,  lip,  chg,  aac, alm1, alm2, localization
    AAT_ECOLI,    0.49, 0.29, 0.48, 0.50, 0.56, 0.24, 0.35, cp
    ACEA_ECOLI,   0.07, 0.40, 0.48, 0.50, 0.54, 0.35, 0.44, cp
    ACEK_ECOLI,   0.56, 0.40, 0.48, 0.50, 0.49, 0.37, 0.46, cp
    ACKA_ECOLI,   0.59, 0.49, 0.48, 0.50, 0.52, 0.45, 0.36, cp
    ADI_ECOLI,    0.23, 0.32, 0.48, 0.50, 0.55, 0.25, 0.35, cp
    ALKH_ECOLI,   0.67, 0.39, 0.48, 0.50, 0.36, 0.38, 0.46, cp
    AMPD_ECOLI,   0.29, 0.28, 0.48, 0.50, 0.44, 0.23, 0.34, cp
    AMY2_ECOLI,   0.21, 0.34, 0.48, 0.50, 0.51, 0.28, 0.39, cp
    APT_ECOLI,    0.20, 0.44, 0.48, 0.50, 0.46, 0.51, 0.57, cp
    ARAC_ECOLI,   0.42, 0.40, 0.48, 0.50, 0.56, 0.18, 0.30, cp)");

  p.output_index = std::nullopt;

  CHECK(d.read_csv(ecoli, p) == 10);
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
}

TEST_CASE_FIXTURE(fixture_ci, "ecoli categories strong")
{
  using namespace ultra;

  std::istringstream ecoli(R"(
    sequence name, mcg,  gvh,  lip,  chg,  aac, alm1, alm2, localization
    AAT_ECOLI,    0.49, 0.29, 0.48, 0.50, 0.56, 0.24, 0.35, cp
    ACEA_ECOLI,   0.07, 0.40, 0.48, 0.50, 0.54, 0.35, 0.44, cp
    ACEK_ECOLI,   0.56, 0.40, 0.48, 0.50, 0.49, 0.37, 0.46, cp
    ACKA_ECOLI,   0.59, 0.49, 0.48, 0.50, 0.52, 0.45, 0.36, cp
    ADI_ECOLI,    0.23, 0.32, 0.48, 0.50, 0.55, 0.25, 0.35, cp
    ALKH_ECOLI,   0.67, 0.39, 0.48, 0.50, 0.36, 0.38, 0.46, cp
    AMPD_ECOLI,   0.29, 0.28, 0.48, 0.50, 0.44, 0.23, 0.34, cp
    AMY2_ECOLI,   0.21, 0.34, 0.48, 0.50, 0.51, 0.28, 0.39, cp
    APT_ECOLI,    0.20, 0.44, 0.48, 0.50, 0.46, 0.51, 0.57, cp
    ARAC_ECOLI,   0.42, 0.40, 0.48, 0.50, 0.56, 0.18, 0.30, cp)");

  p.output_index = std::nullopt;
  p.data_typing = src::typing::strong;

  CHECK(d.read_csv(ecoli, p) == 10);
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
}

TEST_CASE_FIXTURE(fixture_ci, "load_csv classification")
{
  using namespace ultra;

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

  p.output_index = 4;
  CHECK(d.read_csv(iris, p) == 10);
  CHECK(d.is_valid());

  CHECK(cs.is_valid());

  CHECK(cs[0].name() == "class");
  CHECK(cs[1].name() == "sepal length");
  CHECK(cs[2].name() == "sepal width");
  CHECK(cs[3].name() == "petal length");
  CHECK(cs[4].name() == "petal width");

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0});

  CHECK(std::ranges::all_of(cs, [](auto c) { return c.domain() == d_double; }));
}

TEST_CASE_FIXTURE(fixture_ci, "load_csv classification strong")
{
  using namespace ultra;

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

  p.output_index = 4;
  p.data_typing = src::typing::strong;
  CHECK(d.read_csv(iris, p) == 10);
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

  CHECK(cs.used_categories() == std::set<symbol::category_t>{0, 1, 2, 3, 4});

  CHECK(std::ranges::all_of(cs, [](auto c) { return c.domain() == d_double; }));
}

}  // TEST_SUITE("COLUMNS_INFO")
