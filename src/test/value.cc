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
  static_assert(std::is_same_v
    <D_IVECTOR, std::variant_alternative_t<d_ivector, value_t>>);
}

TEST_CASE("Base")
{
  using namespace ultra;

  value_t v1;
  std::ostringstream out;

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

  SUBCASE("Address value")
  {
    v1 = 345_addr;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_address);
    CHECK(!basic_data_type(v1));
    CHECK(!numerical_data_type(v1));

    out << v1;
    CHECK(out.str()
          == "[" + std::to_string(as_integer(std::get<D_ADDRESS>(v1))) + "]");
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

  SUBCASE("Vector value")
  {
    const std::vector v = {0, 1, 2, 3, 4, 5};
    v1 = v;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_ivector);
    CHECK(!basic_data_type(v1));
    CHECK(!numerical_data_type(v1));

    out << v1;
    const auto os(out.str());
    CHECK(os.front() == '{');
    CHECK(os.back() == '}');
    CHECK(os.length() == 2*v.size() - 1 + 2);
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

TEST_CASE("Serialization")
{
  using namespace ultra;

  std::stringstream ss;

  SUBCASE("Empty value")
  {
    value_t v1;
    CHECK(save(ss, v1));
    CHECK(ss.str() == std::to_string(v1.index()));
  }

  SUBCASE("String value")
  {
    const D_STRING s("dummy");
    value_t v1(s);
    CHECK(save(ss, v1));
    CHECK(ss.str() == std::to_string(v1.index()) + " " + s);
  }

  SUBCASE("Integer value")
  {
    const D_INT i(123);
    value_t v1(i);
    CHECK(save(ss, v1));
    CHECK(ss.str() == std::to_string(v1.index()) + " " + std::to_string(i));
  }

  SUBCASE("Double value")
  {
    const D_DOUBLE i(123.0);
    value_t v1(i);
    CHECK(save(ss, v1));

    std::ostringstream oss;
    CHECK(save_float_to_stream(oss, i));

    CHECK(ss.str() == std::to_string(v1.index()) + " " + oss.str());
  }

  SUBCASE("Address value")
  {
    const D_ADDRESS a(345_addr);
    value_t v1(a);
    CHECK(save(ss, v1));
    CHECK(ss.str() == std::to_string(v1.index()) + " "
          + std::to_string(as_integer(a)));
  }

  SUBCASE("Vector value")
  {
    const D_IVECTOR v = {1, 2, 3};
    value_t v1(v);
    CHECK(save(ss, v1));

    CHECK(ss.str() == std::to_string(v1.index()) + " 3 1 2 3");
  }

  SUBCASE("Empty Vector value")
  {
    const D_IVECTOR v;
    value_t v1(v);
    CHECK(save(ss, v1));

    CHECK(ss.str() == std::to_string(v1.index()) + " 0");
  }

  // `d_nullary` and `d_variable` require a symbol set and aren't checked.
}

}  // TEST_SUITE("VALUE_T")
