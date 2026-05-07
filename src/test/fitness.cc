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

#include "kernel/fitness.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <sstream>
#include <vector>

TEST_SUITE("FITNESS")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  static_assert(Fitness<double>);
  static_assert(Fitness<int>);

  static_assert(MultiDimFitness<fitnd>);
  static_assert(MultiDimFitness<std::vector<double>>);
  static_assert(MultiDimFitness<std::vector<int>>);

  static_assert(!MultiDimFitness<double>);
  static_assert(!MultiDimFitness<std::vector<std::string>>);
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

TEST_CASE("Range interface")
{
  using namespace ultra;

  fitnd f{1.0, 2.0, 3.0};

  CHECK(std::ranges::size(f) == 3);

  std::ranges::fill(f, 4.0);
  CHECK(f == fitnd{4.0, 4.0, 4.0});

  const fitnd cf{1.0, 2.0, 3.0};
  CHECK(std::accumulate(std::ranges::begin(cf), std::ranges::end(cf), 0.0)
        == doctest::Approx(6.0));
}

TEST_CASE("Serialisation")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Round-trip")
  {
    const fitnd f{0.0, 1.0, 2.0,
                  std::numeric_limits<fitnd::value_type>::lowest(),
                  std::numeric_limits<fitnd::value_type>::infinity()};

    CHECK(save(ss, f));

    fitnd f2;
    CHECK(f2.size() == 0);
    CHECK(load(ss, &f2));

    CHECK(f2.size() == f.size());
    CHECK(f == f2);
  }

  SUBCASE("Length")
  {
    CHECK(save(ss, fitnd{1.0, 2.0, 3.0}));
    CHECK(ss.str().starts_with("3 "));
  }

  SUBCASE("Composability")
  {
    CHECK(save(ss, fitnd{1.0, 2.0, 3.0}));
    ss << ' ';
    CHECK(save(ss, fitnd{4.0, 5.0}));

    fitnd a, b;
    CHECK(load(ss, &a));
    CHECK(load(ss, &b));

    CHECK(almost_equal(a, fitnd{1.0, 2.0, 3.0}));
    CHECK(almost_equal(b, fitnd{4.0, 5.0}));
  }

  SUBCASE("Failed load doesn't modify destination")
  {
    ss.str("3 1 2 invalid");

    fitnd f{9.0, 9.0};
    CHECK(!load(ss, &f));
    CHECK(f == fitnd{9.0, 9.0});
  }
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

    SUBCASE("Value between parentheses")
    {
      ss << f;

      const auto str(ss.str());
      CHECK(str.front() == '(');
      CHECK(str.back() == ')');

      fitnd f1;
      ss >> f1;

      CHECK(f1.size() == f.size());
      CHECK(almost_equal(f1, f));
    }

    SUBCASE("Value without parentheses")
    {
      ss << val;

      fitnd f1;
      ss >> f1;

      CHECK(f1.size() == f.size());
      CHECK(almost_equal(f1, f));
    }

    SUBCASE("Invalid input does not modify destination")
    {
      ss.str("(1, invalid)");

      fitnd f1(f);

      const bool result(ss >> f1);
      CHECK(!result);
      CHECK(f1 == f);
    }
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
  CHECK(2.0 * f1 == f2);

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

  SUBCASE("Scalar")
  {
    CHECK(distance(1.0, 4.5) == doctest::Approx(3.5));
    CHECK(distance(4.5, 1.0) == doctest::Approx(3.5));

    CHECK(distance(1, 4) == doctest::Approx(3.0));
    CHECK(distance(4, 1) == doctest::Approx(3.0));

    CHECK(distance(std::numeric_limits<int>::lowest(),
                   std::numeric_limits<int>::max()) > 0.0);
  }

  SUBCASE("fitnd")
  {
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

  SUBCASE("vector")
  {
    const std::vector a{1, 2, 3};
    const std::vector b{3, 2, 1};

    CHECK(ultra::distance(a, b) == doctest::Approx(4.0));
  }
}

TEST_CASE("Generic multidimensional fitness")
{
  using namespace ultra;

  const std::vector<double> f1{3.0, 2.0, 1.0};
  const std::vector<double> f2{2.0, 2.0, 1.0};
  const std::vector<double> f3{3.0, 3.0, 0.0};

  CHECK(dominating(f1, f2));
  CHECK(!dominating(f2, f1));

  CHECK(!dominating(f1, f3));
  CHECK(!dominating(f3, f1));

  CHECK(ultra::distance(f1, f2) == doctest::Approx(1.0));
  CHECK(ultra::distance(f1, f3) == doctest::Approx(2.0));

  CHECK(almost_equal(f1, f1));
  CHECK(!almost_equal(f1, f2));
}

}  // TEST_SUITE
