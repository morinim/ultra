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

namespace ultra::src::internal
{
bool compatible(const function::param_data_types &,
                const std::vector<std::string> &, const columns_info &);
}

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

  // Given a nominal column with repeated states, when setup_symbols() is
  // called then exactly one terminal per distinct state is created.
  SUBCASE("Duplicate nominal states generate a single attribute terminal")
  {
    std::istringstream duplicated_value(debug::duplicated_value);

    src::problem p(duplicated_value, src::dataframe::params().header());

    CHECK(p.ready());

    // Exactly two attribute terminals: "red" and "blue"
    CHECK(p.sset.categories() == 2);
    CHECK(p.sset.terminals(0) == 0);
    CHECK(p.sset.terminals(1) == 3);  // "red", "blue" and "COLOUR" variable
  }
}

TEST_CASE("compatible")
{
  using namespace ultra;
  using ultra::src::internal::compatible;

  SUBCASE("Weak - 0 output")
  {
    src::dataframe d(debug::abalone_table);

    symbol::category_t sex(0), length(0), diameter(0), height(0), rings(1);
    CHECK(compatible({sex}, {"sex"}, d.columns));
    CHECK(compatible({sex}, {"numeric"}, d.columns));
    CHECK(compatible({length}, {"length"}, d.columns));
    CHECK(compatible({length}, {"numeric"}, d.columns));
    CHECK(compatible({diameter}, {"diameter"}, d.columns));
    CHECK(compatible({diameter}, {"numeric"}, d.columns));
    CHECK(compatible({height}, {"height"}, d.columns));
    CHECK(compatible({height}, {"numeric"}, d.columns));
    CHECK(compatible({rings}, {"rings"}, d.columns));
    CHECK(compatible({rings}, {"integer"}, d.columns));
  }

  SUBCASE("Weak - 8 output")
  {
    src::dataframe::params params;
    params.output_index = 8;

    src::dataframe d(debug::abalone_table, params);

    symbol::category_t sex(1), length(2), diameter(2), height(2), rings(0);
    CHECK(compatible({sex}, {"sex"}, d.columns));
    CHECK(compatible({sex}, {"string"}, d.columns));
    CHECK(compatible({length}, {"length"}, d.columns));
    CHECK(compatible({length}, {"numeric"}, d.columns));
    CHECK(compatible({diameter}, {"diameter"}, d.columns));
    CHECK(compatible({diameter}, {"numeric"}, d.columns));
    CHECK(compatible({height}, {"height"}, d.columns));
    CHECK(compatible({height}, {"numeric"}, d.columns));
    CHECK(compatible({rings}, {"rings"}, d.columns));
    CHECK(compatible({rings}, {"integer"}, d.columns));
  }
}

}  // TEST_SUITE("SRC::PROBLEM")
