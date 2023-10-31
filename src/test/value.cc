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

#include "kernel/value.h"
#include "kernel/nullary.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("VALUE_T")
{

TEST_CASE("Base")
{
  using namespace ultra;

  value_t v1;
  std::stringstream out;

  SUBCASE("Empty value")
  {
    CHECK(!has_value(v1));
    CHECK(v1.index() == d_void);

    out << v1;
    CHECK(out.str() == "{}");
  }

  SUBCASE("String value")
  {
    v1 = std::string("dummy");
    CHECK(has_value(v1));
    CHECK(v1.index() == d_string);

    out << v1;
    CHECK(out.str() == std::get<D_STRING>(v1));
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
    CHECK(get_if_nullary(v1));

    out << v1;
    CHECK(out.str() == hw.to_string());
  }

  SUBCASE("Integer value")
  {
    v1 = 1;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_int);

    out << v1;
    CHECK(out.str() == std::to_string(std::get<D_INT>(v1)));
  }

  SUBCASE("Double value")
  {
    v1 = 1.0;
    CHECK(has_value(v1));
    CHECK(v1.index() == d_double);

    out << v1;
    CHECK(almost_equal(std::stod(out.str()), std::get<D_DOUBLE>(v1)));

    value_t v2(1.00000000000001);
    CHECK(has_value(v2));
    CHECK(v2.index() == d_double);
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
