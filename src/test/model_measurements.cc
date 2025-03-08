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

#include <cstdlib>
#include <sstream>

#include "kernel/model_measurements.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("MODEL MEASUREMENTS")
{

TEST_CASE("Base")
{
  using namespace ultra;

  CHECK(model_measurements(-5.0, .8) >= model_measurements(-5.0, .8));
  CHECK(model_measurements(-5.0, .8) <= model_measurements(-5.0, .8));
  CHECK(model_measurements(-5.0, .8) > model_measurements(-10.0, .8));
  CHECK(model_measurements(-10.0, .8) < model_measurements(-5.0, .8));

  CHECK(model_measurements(fitnd{0.0, 1.0}, .8)
        > model_measurements(fitnd{0.0, 0.0}, .8));
  CHECK(model_measurements(fitnd{0.0, 1.0}, .9)
        > model_measurements(fitnd{0.0, 1.0}, .8));

  CHECK(!(model_measurements(5.0, .8) > model_measurements(4.0, .9)));
  CHECK(!(model_measurements(4.0, .9) > model_measurements(5.0, .8)));
  CHECK(!(model_measurements(5.0, .8) >= model_measurements(4.0, .9)));
  CHECK(!(model_measurements(4.0, .9) >= model_measurements(5.0, .8)));
  CHECK(!(model_measurements(5.0, .8) < model_measurements(4.0, .9)));
  CHECK(!(model_measurements(4.0, .9) < model_measurements(5.0, .8)));
  CHECK(!(model_measurements(5.0, .8) <= model_measurements(4.0, .9)));
  CHECK(!(model_measurements(4.0, .9) <= model_measurements(5.0, .8)));
  CHECK(model_measurements(4.0, .9) != model_measurements(5.0, .8));

  model_measurements<double> empty;
  CHECK(empty.empty());

  model_measurements<double> partially_empty;
  partially_empty.fitness = 8.0;
  CHECK(!partially_empty.empty());

  CHECK(model_measurements(10.0, .9) > empty);
  CHECK(partially_empty > empty);
  CHECK(model_measurements(10.0, .9) > partially_empty);
  CHECK(model_measurements(8.0, .9) > partially_empty);
  CHECK(!(model_measurements(7.0, .9) < partially_empty));
  CHECK(!(model_measurements(7.0, .9) > partially_empty));

  model_measurements<double> partially_empty2, partially_empty3;
  partially_empty2.accuracy = .75;
  partially_empty3.accuracy = .90;

  CHECK(partially_empty2 > empty);
  CHECK(partially_empty2 < partially_empty3);
}

TEST_CASE("Serialization")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Normal")
  {
    model_measurements m(-5.0, .8);

    CHECK(m.save(ss));

    decltype(m) m1;
    CHECK(m1.load(ss));

    CHECK(*m.fitness == doctest::Approx(*m1.fitness));
    CHECK(*m.accuracy == doctest::Approx(*m1.accuracy));
  }

  SUBCASE("Empty")
  {
    model_measurements<double> empty;
    CHECK(empty.save(ss));

    decltype(empty) m1;
    CHECK(m1.load(ss));

    CHECK(m1.empty());
  }
}

}  // TEST_SUITE("MODEL MEASUREMENTS")
