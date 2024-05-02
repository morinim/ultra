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

#include "kernel/value.h"
#include "kernel/nullary.h"
#include "kernel/gp/src/variable.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("VALUE_T")
{

TEST_CASE("Correct mapping")
{
  using namespace ultra;

  // Consider `return {};`... this is void not something default initialized!
  static_assert(d_void == 0,
                "D_VOID must be the first alternative of the variant");

  static_assert(std::is_same_v
    <D_VOID, std::variant_alternative_t<d_void, value_t>>);
  static_assert(std::is_same_v
    <D_INT, std::variant_alternative_t<d_int, value_t>>);
  static_assert(std::is_same_v
    <D_DOUBLE, std::variant_alternative_t<d_double, value_t>>);
  static_assert(std::is_same_v
    <D_STRING, std::variant_alternative_t<d_string, value_t>>);
  static_assert(std::is_same_v
    <const D_NULLARY *, std::variant_alternative_t<d_nullary, value_t>>);
  static_assert(std::is_same_v
    <D_ADDRESS, std::variant_alternative_t<d_address, value_t>>);
  static_assert(std::is_same_v
    <const D_VARIABLE *, std::variant_alternative_t<d_variable, value_t>>);
}

TEST_CASE("Base")
{
  using namespace ultra;

  value_t v1;
  std::stringstream out;

  SUBCASE("Empty value")
  {
    CHECK(!has_value(v1));
    CHECK(v1.index() == d_void);
    CHECK(basic_data_type(v1));
    CHECK(!numerical_data_type(v1));

    out << v1;
    CHECK(out.str() == "{}");
  }

  SUBCASE("String value")
  {
    v1 = std::string("dummy");
    CHECK(has_value(v1));
    CHECK(v1.index() == d_string);
    CHECK(basic_data_type(v1));
    CHECK(!numerical_data_type(v1));

    out << v1;
    CHECK(out.str() == "\"" + std::get<D_STRING>(v1) + "\"");
  }

  SUBCASE("Nullary value")
  {
    class greetings : public nullary
    {
    public:
      using nullary::nullary;

      value_t eval() const override
      {
        std::cout << "Hello world\n";
        return {};
      }
    };

    greetings hw("greetings");
    v1 = &hw;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_nullary);
    CHECK(!basic_data_type(v1));
    CHECK(!numerical_data_type(v1));
    CHECK(get_if_nullary(v1));

    out << v1;
    CHECK(out.str() == hw.to_string());
  }

  SUBCASE("Integer value")
  {
    v1 = 1;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_int);
    CHECK(basic_data_type(v1));
    CHECK(numerical_data_type(v1));

    out << v1;
    CHECK(out.str() == std::to_string(std::get<D_INT>(v1)));
  }

  SUBCASE("Double value")
  {
    v1 = 1.0;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_double);
    CHECK(basic_data_type(v1));
    CHECK(numerical_data_type(v1));

    out << v1;
    CHECK(almost_equal(std::stod(out.str()), std::get<D_DOUBLE>(v1)));

    value_t v2(1.00000000000001);
    CHECK(has_value(v2));
    CHECK(v2.index() == d_double);
  }

  SUBCASE("Variable")
  {
    const std::string name("X2");
    src::variable var(2, name);
    v1 = &var;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_variable);
    CHECK(!basic_data_type(v1));
    CHECK(!numerical_data_type(v1));

    out << v1;
    CHECK(out.str() == name);
  }

  SUBCASE("Different types comparison")
  {
    value_t v2(1.0);
    v1 = 1;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_int);
    const bool type_diff(v1 != v2);
    CHECK(type_diff);
  }
}

}  // TEST_SUITE("VALUE_T")
