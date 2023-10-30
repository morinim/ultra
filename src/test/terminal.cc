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

#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/primitive/string.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("TERMINAL")
{

TEST_CASE("Real")
{
  using namespace ultra;

  const D_DOUBLE m(0.0), s(1.0);
  real::number r(m, s);
  CHECK(r.is_valid());
  CHECK(r.category() == symbol::default_category);
  CHECK(almost_equal(std::get<D_DOUBLE>(r.min()), m));
  CHECK(almost_equal(std::get<D_DOUBLE>(r.sup()), s));

  std::vector<D_DOUBLE> v(1000);
  std::ranges::generate(v, [&r] { return std::get<D_DOUBLE>(r.instance()); });

  CHECK(std::ranges::all_of(v, [m, s](auto x) { return m <= x && x < s; }));

  const D_DOUBLE mean(std::accumulate(v.begin(), v.end(), 0.0) / v.size());
  CHECK((s - m) * .4 <= mean);
  CHECK(mean <= (s - m) * .6);
}

TEST_CASE("IReal")
{
  using namespace ultra;

  const int m(0), s(10);
  real::integer r(m, s);
  CHECK(r.is_valid());
  CHECK(r.category() == symbol::default_category);
  CHECK(static_cast<int>(std::get<D_DOUBLE>(r.min())) == m);
  CHECK(static_cast<int>(std::get<D_DOUBLE>(r.sup())) == s);

  std::vector<D_DOUBLE> v(1000);
  std::ranges::generate(v, [&r] { return std::get<D_DOUBLE>(r.instance()); });

  CHECK(std::ranges::all_of(v, [m, s](auto x) { return m <= x && x < s; }));

  const D_DOUBLE mean(std::accumulate(v.begin(), v.end(), 0.0) / v.size());
  CHECK((s - m) * .4 <= mean);
  CHECK(mean <= (s - m) * .6);
}

TEST_CASE("INTEGER")
{
  using namespace ultra;

  const D_INT m(0), s(256);
  integer::number r(m, s);
  CHECK(r.is_valid());
  CHECK(r.category() == symbol::default_category);

  std::vector<D_INT> v(1000);
  std::ranges::generate(v, [&r] { return std::get<D_INT>(r.instance()); });

  CHECK(std::ranges::all_of(v, [m, s](auto x) { return m <= x && x < s; }));

  const D_DOUBLE mean(std::accumulate(v.begin(), v.end(), 0.0) / v.size());
  CHECK((s - m) * .4 <= mean);
  CHECK(mean <= (s - m) * .6);
}

TEST_CASE("Nullary")
{
  using namespace ultra;

  class variable : public nullary
  {
  public:
    using nullary::nullary;

    [[nodiscard]] value_t eval() const override { return val; }

    int val {};
  };

  variable v("var");
  CHECK(v.is_valid());
  CHECK(v.category() == symbol::default_category);

  for (int i(0); i < 100; ++i)
  {
    v.val = i;

    CHECK(std::get<D_INT>(v.eval()) == i);
  }
}

}  // TEST_SUITE("REAL")
