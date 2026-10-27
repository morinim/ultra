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

#include "test/debug_datasets.h"

#include "kernel/gp/src/problem.h"
#include "kernel/exceptions.h"
#include "kernel/gp/primitive/real.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <sstream>
#include <stdexcept>

namespace ultra::src::internal
{
bool compatible(const function::param_data_types &,
                const std::vector<std::string> &, const columns_info &);
}

TEST_SUITE("src::problem")
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

    CHECK(p.sset.decode("FDIV"));
    CHECK(p.sset.decode("FLN"));
    CHECK(p.sset.decode("FEXP"));
    CHECK(p.sset.decode("FSIN"));

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

    CHECK(p.sset.decode("FDIV"));
    CHECK(p.sset.decode("FSIN"));
    CHECK(p.sset.decode("FLN") == nullptr);
    CHECK(p.sset.decode("FEXP") == nullptr);

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

    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error() == src::problem::readiness_error::missing_functions);

    // Exactly two attribute terminals: "red" and "blue"
    CHECK(p.sset.categories() == 2);
    CHECK(p.sset.terminals(0) == 0);
    CHECK(p.sset.terminals(1) == 3);  // "red", "blue" and "COLOUR" variable
  }

  SUBCASE("Fallback variable naming for unnamed columns")
  {
    std::istringstream sr_stream(debug::sr);
    src::problem p(sr_stream);

    CHECK(p.is_valid());
    CHECK(p.variables() == 1);
    CHECK(p.classes() == 0);
    CHECK(p.sset.decode("X1"));
  }

  SUBCASE("Dataset with insufficient columns throws")
  {
    src::dataframe empty_df;
    CHECK_THROWS_AS(src::problem p(empty_df), exception::insufficient_data);

    const std::string single_col("TARGET\n1.0\n2.0\n");
    std::istringstream stream(single_col);
    src::dataframe one_col_df(stream, src::dataframe::params().header());
    CHECK_THROWS_AS(src::problem p(one_col_df), exception::insufficient_data);
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

  SUBCASE("Negative matching and empty signatures")
  {
    src::dataframe::params params;
    params.output_index = 8;
    src::dataframe d(debug::abalone_table, params);

    symbol::category_t sex(1), rings(0);

    // Empty instance and pattern are compatible.
    CHECK(compatible({}, {}, d.columns));

    // Category mismatch.
    CHECK(!compatible({sex}, {"rings"}, d.columns));
    // Domain mismatch.
    CHECK(!compatible({sex}, {"numeric"}, d.columns));
    CHECK(!compatible({rings}, {"string"}, d.columns));

    // Multi-element signature matching.
    CHECK(compatible({sex, rings}, {"string", "integer"}, d.columns));
    CHECK(compatible({sex, rings}, {"sex", "rings"}, d.columns));
    CHECK(!compatible({sex, rings}, {"string", "string"}, d.columns));
    CHECK(!compatible({sex, rings}, {"integer", "integer"}, d.columns));

    // Non-existent column name throws out_of_range.
    CHECK_THROWS_AS(compatible({sex}, {"non_existent_col"}, d.columns),
                    std::out_of_range);
  }
}

TEST_CASE("disabled symbol initialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream wine(debug::wine);
  src::problem p(wine, src::dataframe::params());

  REQUIRE(p.sset.terminals() > 0);

  std::istringstream wine2(debug::wine);
  src::problem disabled(src::dataframe(wine2), symbol_init::disabled);

  CHECK(disabled.is_valid());
  CHECK(disabled.sset.terminals() == 0);
  CHECK(disabled.sset.functions() == 0);

  const auto ready(disabled.ready());
  REQUIRE(!ready);
  CHECK(ready.error() == src::problem::readiness_error::missing_functions);
}

TEST_CASE("readiness diagnostics")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  SUBCASE("empty default problem")
  {
    src::problem p;

    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error() == src::problem::readiness_error::empty_training_set);
  }

  SUBCASE("missing functions")
  {
    std::istringstream wine(debug::wine);
    src::problem p(wine, src::dataframe::params());

    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error() == src::problem::readiness_error::missing_functions);
  }

  SUBCASE("insufficient terminals")
  {
    std::istringstream wine(debug::wine);
    src::problem p(src::dataframe(wine), symbol_init::disabled);

    p.insert<real::add>();

    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error()
          == src::problem::readiness_error::insufficient_terminals);
  }

  SUBCASE("complete symbol set")
  {
    std::istringstream wine(debug::wine);
    src::problem p(wine, src::dataframe::params());
    p.setup_symbols();

    CHECK(p.ready().has_value());
  }

  SUBCASE("invalid problem")
  {
    std::istringstream wine(debug::wine);
    src::problem p(wine, src::dataframe::params());
    p.setup_symbols();
    REQUIRE(p.ready().has_value());

    p.params.alps.max_layers = 1;
    CHECK(!p.is_valid());

    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error() == src::problem::readiness_error::invalid_problem);
  }

  SUBCASE("missing category terminals")
  {
    std::istringstream wine(debug::wine);
    src::problem p(src::dataframe(wine), symbol_init::disabled);

    // Provide a function and terminal for category 0 only.
    p.insert<real::add>(symbol::category_t(0));
    p.insert<real::literal>(1.0, symbol::category_t(0));

    // Category 1 is present in wine training features but has no terminals.
    const auto ready(p.ready());
    REQUIRE(!ready);
    CHECK(ready.error()
          == src::problem::readiness_error::missing_category_terminals);
  }
}

TEST_CASE("construct from moved base problem")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream wine(debug::wine);
  src::problem source(wine);

  source.params.population.individuals = 123;
  source.params.evolution.generations = 7;

  const auto source_terminals(source.sset.terminals());
  const auto source_functions(source.sset.functions());

  ultra::problem base;
  using std::swap;
  swap(base.params, source.params);
  swap(base.sset, source.sset);

  std::istringstream iris(debug::iris);
  src::dataframe::params p;
  p.output_index = 4;
  src::problem moved(std::move(base), src::dataframe(iris, p));

  REQUIRE(moved.is_valid());
  CHECK(moved.params.population.individuals == 123);
  CHECK(moved.params.evolution.generations == 7);
  CHECK(moved.sset.terminals() == source_terminals);
  CHECK(moved.sset.functions() == source_functions);
  CHECK(moved.data[src::dataset_t::training].size() == debug::IRIS_COUNT);

  // Postconditions on base problem.
  CHECK(base.is_valid());
  CHECK(base.sset.terminals() == 0);
  CHECK(base.sset.functions() == 0);
}

TEST_CASE("binary classification")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream gender_stream(debug::gender);
  src::dataframe::params params;
  params.header();
  params.output_index = 0;

  src::problem p(gender_stream, params);
  p.setup_symbols();

  CHECK(p.is_valid());
  CHECK(p.classes() == 2);
  CHECK(p.classification());
  CHECK(p.variables() == 3);
  CHECK(p.ready().has_value());
}

TEST_CASE("symbol_init flags and ephemerals")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  SUBCASE("symbol_init::all generates ephemeral terminals")
  {
    std::istringstream wine(debug::wine);
    src::problem p(src::dataframe(wine), symbol_init::all);

    CHECK(p.is_valid());
    CHECK(p.ready().has_value());
    // Wine has 10 double variables (cat 0) and 1 integer variable (cat 1).
    // symbol_init::all also generates ephemerals for each used category:
    // real::number for cat 0 (total 11) and integer::number for cat 1 (total 2).
    CHECK(p.sset.terminals(0) == 11);
    CHECK(p.sset.terminals(1) == 2);
  }

  SUBCASE("symbol_init::variables alone")
  {
    std::istringstream wine(debug::wine);
    src::problem p(src::dataframe(wine), symbol_init::variables);

    CHECK(p.sset.terminals(0) == 10);
    CHECK(p.sset.terminals(1) == 1);
    CHECK(p.sset.functions(0) == 0);
    CHECK(p.sset.functions(1) == 0);
  }
}

TEST_CASE("multi_dataset schema cloning and problem validity")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream wine(debug::wine);
  src::problem p(wine);

  const auto &train_cols(p.data[src::dataset_t::training].columns);
  const auto &val_cols(p.data[src::dataset_t::validation].columns);
  const auto &test_cols(p.data[src::dataset_t::test].columns);

  CHECK(val_cols.size() == train_cols.size());
  CHECK(test_cols.size() == train_cols.size());
  CHECK(p.data[src::dataset_t::validation].empty());
  CHECK(p.data[src::dataset_t::test].empty());

  for (std::size_t i(0); i < train_cols.size(); ++i)
  {
    CHECK(val_cols[i].name() == train_cols[i].name());
    CHECK(val_cols[i].domain() == train_cols[i].domain());
    CHECK(test_cols[i].name() == train_cols[i].name());
    CHECK(test_cols[i].domain() == train_cols[i].domain());
  }

  // Idempotency: re-running setup_symbols does not duplicate symbols.
  p.setup_symbols();
  const auto term_count(p.sset.terminals());
  const auto func_count(p.sset.functions());
  p.setup_symbols();
  CHECK(p.sset.terminals() == term_count);
  CHECK(p.sset.functions() == func_count);
}

}  // TEST_SUITE
