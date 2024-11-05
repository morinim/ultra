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
  static_assert(Fitness<fitnd>);
}

TEST_CASE("Comparison")
{
  using namespace ultra;

  fitnd fit2d(with_size(2));
  fitnd fit3d(with_size(3));
  fitnd fit4d(with_size(4));

  fitnd f1{3.0, 0.0, 0.0}, f2{2.0, 1.0, 0.0}, f3{2.0, 0.0, 0.0};

  CHECK(fit2d.size() == 2);
  CHECK(fit3d.size() == 3);
  CHECK(fit4d.size() == 4);

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

  CHECK(almost_equal(f1, f1));
  CHECK(!almost_equal(f1, f2));
  CHECK(almost_equal(
          fitnd{std::numeric_limits<fitnd::value_type>::infinity()},
          fitnd{std::numeric_limits<fitnd::value_type>::infinity()}));
}

TEST_CASE("Serialisation")
{
  using namespace ultra;

  const fitnd f{0.0, 1.0, 2.0,
                std::numeric_limits<fitnd::value_type>::lowest(),
                std::numeric_limits<fitnd::value_type>::infinity()};

  std::stringstream ss;

  CHECK(save(ss, f));

  fitnd f2;
  CHECK(f2.size() == 0);
  CHECK(load(ss, &f2));

  CHECK(f2.size() == f.size());
  CHECK(f == f2);
}

TEST_CASE("Input/Output")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Multidimensional fitness")
  {
    const fitnd f{0.0, 1.0, 2.5,
                  std::numeric_limits<fitnd::value_type>::infinity()};

    ss << f;

    const auto str(ss.str());
    CHECK(str == "(0, 1, 2.5, inf)");

    fitnd f1;
    ss >> f1;
    CHECK(f1.size() == f.size());
    CHECK(almost_equal(f1, f));
  }

  SUBCASE("Scalar fitness")
  {
    const auto val(std::numeric_limits<fitnd::value_type>::lowest());
    const fitnd f{val};
    ss << f;

    // Value between parentheses.
    const auto str(ss.str());
    CHECK(str.front() == '(');
    CHECK(str.back() == ')');

    fitnd f1;
    ss >> f1;
    CHECK(f1.size() == f.size());
    CHECK(almost_equal(f1, f));

    // Value without parentheses.
    ss << val;
    fitnd f2;
    ss >> f2;
    CHECK(f2.size() == f.size());
    CHECK(almost_equal(f2, f));
  }
}

TEST_CASE("Operators")
{
  using namespace ultra;

  fitnd x{2.0, 4.0, 8.0};
  fitnd f1{2.0, 4.0, 8.0};
  fitnd f2{4.0, 8.0, 16.0};
  fitnd inf(with_size(3), std::numeric_limits<fitnd::value_type>::infinity());

  x += x;
  CHECK(x == f2);

  CHECK(x / 2.0 == f1);

  CHECK(f1 * 2.0 == f2);

  x = f1 * fitnd{2.0, 2.0, 2.0};
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

  CHECK(ultra::isfinite(x));
  CHECK(!ultra::isfinite(inf));
}

TEST_CASE("Joining")
{
  using namespace ultra;

  const fitnd f1{1.0, 2.0, 3.0}, f2{4.0, 5.0, 6.0};

  const fitnd f3(combine(f1, f2));
  const fitnd f4{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  CHECK(f3 == f4);
}

TEST_CASE("Distance")
{
  using namespace ultra;

  fitnd f1{1.0, 2.0, 3.0}, f2{-4.0, -5.0, -6.0};

  CHECK(distance(f1, f1) == doctest::Approx(0.0));
  CHECK(distance(f2, f2) == doctest::Approx(0.0));

  CHECK(distance(f1, f2) == doctest::Approx(distance(f2, f1)));

  fitnd f3{1.0, 1.0, 1.0}, f4{3.0, 2.0, 3.0};
  const auto d1(distance(f1, f2));
  const auto d2(distance(f3, f4));

  CHECK(distance(combine(f1, f3), combine(f2, f4))
        == doctest::Approx(d1 + d2));

  CHECK(distance(f1, f3) < distance(f2, f3));
  CHECK(distance(f1, f4) == doctest::Approx(2.0));
}

}  // TEST_SUITE("FITNESS")
