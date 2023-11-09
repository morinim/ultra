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

#include "kernel/distribution.h"
#include "kernel/random.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("DISTRIBUTION")
{

TEST_CASE("Base")
{
  using namespace ultra;

  distribution<double> d;

  CHECK(d.size() == 0);

  d.add(2.0);
  d.add(4.0);
  d.add(4.0);
  d.add(4.0);
  d.add(5.0);
  d.add(5.0);
  d.add(7.0);
  d.add(9.0);
  CHECK(d.size() == 8);

  d.add(std::numeric_limits<double>::quiet_NaN());
  CHECK(d.size() == 8);

  CHECK(d.min() == doctest::Approx(2.0));
  CHECK(d.max() == doctest::Approx(9.0));
  CHECK(d.mean() == doctest::Approx(5.0));
  CHECK(d.variance() == doctest::Approx(4.0));
  CHECK(d.standard_deviation() == doctest::Approx(2.0));
}

TEST_CASE("Serialization")
{
  using namespace ultra;
  distribution<double> d;

  for (unsigned i(0); i < 10000; ++i)
    d.add(random::sup(10.0));

  const auto rif_min(d.min());
  const auto rif_max(d.max());
  const auto rif_mean(d.mean());
  const auto rif_variance(d.variance());

  CHECK(4.5 <= rif_mean);
  CHECK(rif_mean <= 5.5);

  std::stringstream s;
  CHECK(d.save(s));

  distribution<double> d1;
  CHECK(d1.load(s));

  CHECK(rif_min == doctest::Approx(d.min()));
  CHECK(rif_max == doctest::Approx(d.max()));
  CHECK(rif_mean == doctest::Approx(d.mean()));
  CHECK(rif_variance == doctest::Approx(d.variance()));
}

}  // TEST_SUITE("FUNCTION")
