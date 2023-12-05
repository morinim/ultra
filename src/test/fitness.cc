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

#include <sstream>
#include <vector>

#include "kernel/fitness.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("FITNESS")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  static_assert(Fitness<double>);
  static_assert(Fitness<int>);
//  static_assert(Fitness<std::vector<double>>);
}

/*
TEST_CASE("Comparison")
{
  using namespace ultra;

  std::valarray<double> fit2d(2, std::numeric_limits<double>::min());
  std::valarray<double> fit3d(3, std::numeric_limits<double>::min());
  std::valarray<double> fit4d(4, std::numeric_limits<double>::min());

  std::valarray<double> f1{3.0, 0.0, 0.0}, f2{2.0, 1.0, 0.0}, f3{2.0, 0.0, 0.0};

  //CHECK(fit2d.size() == 2);
  //CHECK(fit3d.size() == 3);
  //CHECK(fit4d.size() == 4);

  CHECK(f1 > f2);
  CHECK(f1 >= f2);
  CHECK(f2 < f1);
  CHECK(f2 <= f1);

  CHECK(f1 != f2);
  CHECK(f2 != f1);

  CHECK(f1 == f1);
  CHECK(f2 == f2);
  CHECK(fit2d == fit2d);

  CHECK(distance(f1, f1) == doctest::Approx(0.0));
  CHECK(distance(f2, f2) == doctest::Approx(0.0));
  CHECK(distance(fit2d, fit2d) == doctest::Approx(0.0));

  CHECK(dominating(f1, fit3d));
  CHECK(!dominating(fit3d, f1));
  CHECK(!dominating(f1, f2));
  CHECK(!dominating(f2, f1));
  CHECK(!dominating(f1, f1));
  CHECK(dominating(f1, f3));
  CHECK(dominating(f2, f3));
}

TEST_CASE("Serialization")
{
  using vita::fitness_t;

  const fitness_t f{1.0, 2.0, 3.0,
                    std::numeric_limits<fitness_t::value_type>::lowest()};

  std::stringstream ss;

  CHECK(f.save(ss));

  fitness_t f2;
  CHECK(f2.size() == 0);
  CHECK(f2.load(ss));

  CHECK(f2.size() == 4);
  CHECK(f == f2);
}

TEST_CASE("Operators")
{
  using vita::fitness_t;
  using vita::with_size;

  fitness_t x{2.0, 4.0, 8.0};
  fitness_t f1{2.0, 4.0, 8.0};
  fitness_t f2{4.0, 8.0, 16.0};
  fitness_t inf(with_size(3),
                std::numeric_limits<fitness_t::value_type>::infinity());

  x += x;
  CHECK(x == f2);

  CHECK(x / 2.0 == f1);

  CHECK(f1 * 2.0 == f2);

  x = f1 * fitness_t{2.0, 2.0, 2.0};
  CHECK(x == f2);

  x += {0.0, 0.0, 0.0};
  CHECK(x == f2);

  x = x / 1.0;
  CHECK(x == f2);

  x = f2 - f1;
  CHECK(x == f1);

  x = x * x;
  x = sqrt(x);
  CHECK(x == f1);

  x = x * -1.0;
  x = abs(x);
  CHECK(f1 == x);

  CHECK(isfinite(x));
  CHECK(!isfinite(inf));
}

TEST_CASE("Joining")
{
  using vita::combine;
  using vita::fitness_t;

  const fitness_t f1{1.0, 2.0, 3.0}, f2{4.0, 5.0, 6.0};

  const fitness_t f3(combine(f1, f2));
  const fitness_t f4{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  CHECK(f3 == f4);
}
TEST_CASE("Distance")
{
  using vita::combine;
  using vita::distance;
  using vita::fitness_t;

  fitness_t f1{1.0, 2.0, 3.0}, f2{-4.0, -5.0, -6.0};

  CHECK(distance(f1, f1) == doctest::Approx(0.0));
  CHECK(distance(f2, f2) == doctest::Approx(0.0));

  CHECK(distance(f1, f2) == doctest::Approx(distance(f2, f1)));

  fitness_t f3{1.0, 1.0, 1.0}, f4{3.0, 2.0, 3.0};
  const auto d1(distance(f1, f2));
  const auto d2(distance(f3, f4));

  CHECK(distance(combine(f1, f3), combine(f2, f4))
        == doctest::Approx(d1 + d2));

  CHECK(distance(f1, f3) < distance(f2, f3));
  CHECK(distance(f1, f4) == doctest::Approx(2.0));
}
*/
}  // TEST_SUITE("FITNESS")
