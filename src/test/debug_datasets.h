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

#include <string>

#if !defined(ULTRA_DEBUG_DATASETS_H)
#define      ULTRA_DEBUG_DATASETS_H

namespace ultra::debug
{

constexpr std::size_t ABALONE_COUNT = 10;
constexpr char abalone[] = R"(
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
  F,0.55,0.44,0.15,0.8945,0.3145,0.151,0.32,19)";

constexpr std::size_t ECOLI_COUNT = 10;
constexpr char ecoli[] = R"(
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
    ARAC_ECOLI,   0.42, 0.40, 0.48, 0.50, 0.56, 0.18, 0.30, cp)";

constexpr std::size_t IRIS_COUNT = 10;
constexpr char iris[] = R"(
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
    5.9,3,5.1,1.8,Iris-virginica)";

constexpr std::size_t IRIS_FULL_COUNT = 150;
constexpr char iris_full[] = R"(
"setosa",5.1,3.5,1.4,0.2
"setosa",4.9,3,1.4,0.2
"setosa",4.7,3.2,1.3,0.2
"setosa",4.6,3.1,1.5,0.2
"setosa",5,3.6,1.4,0.2
"setosa",5.4,3.9,1.7,0.4
"setosa",4.6,3.4,1.4,0.3
"setosa",5,3.4,1.5,0.2
"setosa",4.4,2.9,1.4,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",5.4,3.7,1.5,0.2
"setosa",4.8,3.4,1.6,0.2
"setosa",4.8,3,1.4,0.1
"setosa",4.3,3,1.1,0.1
"setosa",5.8,4,1.2,0.2
"setosa",5.7,4.4,1.5,0.4
"setosa",5.4,3.9,1.3,0.4
"setosa",5.1,3.5,1.4,0.3
"setosa",5.7,3.8,1.7,0.3
"setosa",5.1,3.8,1.5,0.3
"setosa",5.4,3.4,1.7,0.2
"setosa",5.1,3.7,1.5,0.4
"setosa",4.6,3.6,1,0.2
"setosa",5.1,3.3,1.7,0.5
"setosa",4.8,3.4,1.9,0.2
"setosa",5,3,1.6,0.2
"setosa",5,3.4,1.6,0.4
"setosa",5.2,3.5,1.5,0.2
"setosa",5.2,3.4,1.4,0.2
"setosa",4.7,3.2,1.6,0.2
"setosa",4.8,3.1,1.6,0.2
"setosa",5.4,3.4,1.5,0.4
"setosa",5.2,4.1,1.5,0.1
"setosa",5.5,4.2,1.4,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",5,3.2,1.2,0.2
"setosa",5.5,3.5,1.3,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",4.4,3,1.3,0.2
"setosa",5.1,3.4,1.5,0.2
"setosa",5,3.5,1.3,0.3
"setosa",4.5,2.3,1.3,0.3
"setosa",4.4,3.2,1.3,0.2
"setosa",5,3.5,1.6,0.6
"setosa",5.1,3.8,1.9,0.4
"setosa",4.8,3,1.4,0.3
"setosa",5.1,3.8,1.6,0.2
"setosa",4.6,3.2,1.4,0.2
"setosa",5.3,3.7,1.5,0.2
"setosa",5,3.3,1.4,0.2
"versicolor",7,3.2,4.7,1.4
"versicolor",6.4,3.2,4.5,1.5
"versicolor",6.9,3.1,4.9,1.5
"versicolor",5.5,2.3,4,1.3
"versicolor",6.5,2.8,4.6,1.5
"versicolor",5.7,2.8,4.5,1.3
"versicolor",6.3,3.3,4.7,1.6
"versicolor",4.9,2.4,3.3,1
"versicolor",6.6,2.9,4.6,1.3
"versicolor",5.2,2.7,3.9,1.4
"versicolor",5,2,3.5,1
"versicolor",5.9,3,4.2,1.5
"versicolor",6,2.2,4,1
"versicolor",6.1,2.9,4.7,1.4
"versicolor",5.6,2.9,3.6,1.3
"versicolor",6.7,3.1,4.4,1.4
"versicolor",5.6,3,4.5,1.5
"versicolor",5.8,2.7,4.1,1
"versicolor",6.2,2.2,4.5,1.5
"versicolor",5.6,2.5,3.9,1.1
"versicolor",5.9,3.2,4.8,1.8
"versicolor",6.1,2.8,4,1.3
"versicolor",6.3,2.5,4.9,1.5
"versicolor",6.1,2.8,4.7,1.2
"versicolor",6.4,2.9,4.3,1.3
"versicolor",6.6,3,4.4,1.4
"versicolor",6.8,2.8,4.8,1.4
"versicolor",6.7,3,5,1.7
"versicolor",6,2.9,4.5,1.5
"versicolor",5.7,2.6,3.5,1
"versicolor",5.5,2.4,3.8,1.1
"versicolor",5.5,2.4,3.7,1
"versicolor",5.8,2.7,3.9,1.2
"versicolor",6,2.7,5.1,1.6
"versicolor",5.4,3,4.5,1.5
"versicolor",6,3.4,4.5,1.6
"versicolor",6.7,3.1,4.7,1.5
"versicolor",6.3,2.3,4.4,1.3
"versicolor",5.6,3,4.1,1.3
"versicolor",5.5,2.5,4,1.3
"versicolor",5.5,2.6,4.4,1.2
"versicolor",6.1,3,4.6,1.4
"versicolor",5.8,2.6,4,1.2
"versicolor",5,2.3,3.3,1
"versicolor",5.6,2.7,4.2,1.3
"versicolor",5.7,3,4.2,1.2
"versicolor",5.7,2.9,4.2,1.3
"versicolor",6.2,2.9,4.3,1.3
"versicolor",5.1,2.5,3,1.1
"versicolor",5.7,2.8,4.1,1.3
"virginica",6.3,3.3,6,2.5
"virginica",5.8,2.7,5.1,1.9
"virginica",7.1,3,5.9,2.1
"virginica",6.3,2.9,5.6,1.8
"virginica",6.5,3,5.8,2.2
"virginica",7.6,3,6.6,2.1
"virginica",4.9,2.5,4.5,1.7
"virginica",7.3,2.9,6.3,1.8
"virginica",6.7,2.5,5.8,1.8
"virginica",7.2,3.6,6.1,2.5
"virginica",6.5,3.2,5.1,2
"virginica",6.4,2.7,5.3,1.9
"virginica",6.8,3,5.5,2.1
"virginica",5.7,2.5,5,2
"virginica",5.8,2.8,5.1,2.4
"virginica",6.4,3.2,5.3,2.3
"virginica",6.5,3,5.5,1.8
"virginica",7.7,3.8,6.7,2.2
"virginica",7.7,2.6,6.9,2.3
"virginica",6,2.2,5,1.5
"virginica",6.9,3.2,5.7,2.3
"virginica",5.6,2.8,4.9,2
"virginica",7.7,2.8,6.7,2
"virginica",6.3,2.7,4.9,1.8
"virginica",6.7,3.3,5.7,2.1
"virginica",7.2,3.2,6,1.8
"virginica",6.2,2.8,4.8,1.8
"virginica",6.1,3,4.9,1.8
"virginica",6.4,2.8,5.6,2.1
"virginica",7.2,3,5.8,1.6
"virginica",7.4,2.8,6.1,1.9
"virginica",7.9,3.8,6.4,2
"virginica",6.4,2.8,5.6,2.2
"virginica",6.3,2.8,5.1,1.5
"virginica",6.1,2.6,5.6,1.4
"virginica",7.7,3,6.1,2.3
"virginica",6.3,3.4,5.6,2.4
"virginica",6.4,3.1,5.5,1.8
"virginica",6,3,4.8,1.8
"virginica",6.9,3.1,5.4,2.1
"virginica",6.7,3.1,5.6,2.4
"virginica",6.9,3.1,5.1,2.3
"virginica",5.8,2.7,5.1,1.9
"virginica",6.8,3.2,5.9,2.3
"virginica",6.7,3.3,5.7,2.5
"virginica",6.7,3,5.2,2.3
"virginica",6.3,2.5,5,1.9
"virginica",6.5,3,5.2,2
"virginica",6.2,3.4,5.4,2.3
"virginica",5.9,3,5.1,1.8
)";

constexpr std::size_t SR_COUNT = 10;
constexpr char sr[] = R"(
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
)";

constexpr std::size_t WINE_COUNT = 10;
constexpr char wine[] = R"(
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
   7.5,0.5, 0.36,6.1,0.071,17,102,0.9978,3.35,0.8, 10.5,5)";

}  // namespace ultra::debug

#endif  // include guard
