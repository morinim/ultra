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

#include <numbers>

#include "kernel/random.h"
#include "kernel/distribution.h"

#define DOCTEST_CONFIG_IMPLEMENT
#include "third_party/doctest/doctest.h"

TEST_SUITE("RANDOM")
{

const unsigned n(10000);

template<std::invocable F, class T> void check_f(F f, T min, T sup)
{
  ultra::distribution<double> d;

  for (unsigned i(0); i < n; ++i)
  {
    const auto num(f());

    CHECK(min <= num);
    CHECK(num < sup);

    d.add(num);
  }

  const double expected_mean(std::midpoint(min, sup));
  CHECK(expected_mean * 0.97 <= d.mean());
  CHECK(d.mean() <= expected_mean * 1.03);
}

TEST_CASE("Between")
{
  using namespace ultra;

  SUBCASE("Floating point")
  {
    const double min(2.0), sup(2024.0);
    check_f([&] { return random::between(min, sup); }, min, sup);
  }

  SUBCASE("Integer")
  {
    const int min(0), sup(128);
    check_f([&] { return random::between(min, sup); }, min, sup);
  }

  SUBCASE("Sup")
  {
    const double sup(4096.0);
    check_f([&] { return random::sup(sup); }, 0.0, sup);
  }
}

TEST_CASE("Element")
{
  using namespace ultra;

  distribution<double> d;

  const std::vector<double> v = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  for (unsigned i(0); i < n; ++i)
    d.add(random::element(v));

  CHECK(5.0 * 0.97 <= d.mean());
  CHECK(d.mean() <= 5.0 * 1.03);
}

TEST_CASE("Boolean")
{
  using namespace ultra;

  distribution<double> d;

  const std::vector<double> v = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  for (unsigned i(0); i < n; ++i)
    d.add(random::boolean());

  CHECK(0.5 * 0.97 <= d.mean());
  CHECK(d.mean() <= 0.5 * 1.03);
}

TEST_CASE("Ephemeral")
{
  using namespace ultra;

  SUBCASE("Uniform distribution")
  {
    const double min(0), sup(127);
    const auto dt(random::distribution::uniform);
    check_f([&] { return random::ephemeral(dt, min, sup); }, min, sup);
  }

  SUBCASE("Normal distribution")
  {
    const double min(10000.0), sup(20000.0);
    const auto dt(random::distribution::normal);

    distribution<double> d;
    for (unsigned i(0); i < n; ++i)
      d.add(random::ephemeral(dt, min, sup));

    const double expected_mean(std::midpoint(min, sup));
    CHECK(expected_mean * 0.97 <= d.mean());
    CHECK(d.mean() <= expected_mean * 1.03);

    const double expected_stddev(sup - min);
    CHECK(expected_stddev * 0.97 <= d.standard_deviation());
    CHECK(d.standard_deviation() <= expected_stddev * 1.03);
  }
}

TEST_CASE("Ring")
{
  using namespace ultra;

  struct triplet
  {
    int base;
    int width;
    int n;
  };

  const std::vector<triplet> ts =
  {
    {500, 100, 1000},
    {0, 100, 1000},
    {900, 200, 1000},
    {500, 500, 1000},
    {500, 499, 1000}
  };

  for (const auto &t : ts)
  {
    distribution<double> d;

    const int left((t.base + t.n - t.width) % t.n);
    const int right((t.base + t.width) % t.n);

    CHECK(0 <= left);
    CHECK(left < t.n);
    CHECK(0 <= right);
    CHECK(right < t.n);

    for (unsigned i(0); i < n; ++i)
    {
      const int num(random::ring(t.base, t.width, t.n));

      bool left_range;
      if (left < t.base)
        left_range = (left <= num && num <= t.base);
      else
        left_range = (left <= num && num < t.n) || num <= t.base;

      bool right_range;
      if (t.base < right)
        right_range = (t.base <= num && num <= right);
      else
        right_range = num <= right || (t.base <= num && num < t.n);

      const bool range_check(left_range || right_range);
      CHECK(range_check);

      if (right_range && num < t.base)
        d.add(num + t.n);
      else if (left_range && num > t.base)
        d.add(num - t.n);
      else
        d.add(num);
    }

    const double expected_mean(
      std::midpoint(left > t.base ? left - t.n : left,
                    right < t.base ? right + t.n : right));

    const auto diff(std::fabs(expected_mean - d.mean()));

    CHECK(diff < t.n * 0.01);
  }
}

}  // TEST_SUITE("RANDOM")


int main(int argc, char** argv)
{
  // One-time deterministic initialisation for this thread RNG.
  ultra::random::engine().seed(1973u);

  doctest::Context ctx;
  ctx.applyCommandLine(argc, argv);
  return ctx.run();
}
