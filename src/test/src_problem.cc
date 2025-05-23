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

#include "test/debug_datasets.h"

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

  std::istringstream wine(debug::wine);

  std::istringstream iris(debug::iris);

  SUBCASE("Weak typing - Symbolic regression")
  {
    src::problem p(wine);
    p.setup_symbols();

    CHECK(p.is_valid());

    CHECK(p.categories() == 2);
    CHECK(p.classes() == 0);
    CHECK(p.variables() == 11);

    CHECK(!p.classification());

    CHECK(p.data.selected().size() == debug::WINE_COUNT);
    CHECK(p.data[src::dataset_t::validation].empty());
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

    CHECK(p.data.selected().size() == debug::WINE_COUNT);
    CHECK(p.data[src::dataset_t::validation].empty());
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

    CHECK(p.data.selected().size() == debug::IRIS_COUNT);
    CHECK(p.data[src::dataset_t::validation].empty());
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

    CHECK(p.data.selected().size() == debug::IRIS_COUNT);
    CHECK(p.data[src::dataset_t::validation].empty());
  }
}

}  // TEST_SUITE("SRC::PROBLEM")
