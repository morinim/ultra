/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "kernel/value.h"
#include "kernel/value_format.h"

#include <format>
#include <string>

namespace
{
  // Concrete nullary for testing (nullary is abstract).
  class test_nullary final : public ultra::nullary
  {
  public:
    using ultra::nullary::nullary;

    [[nodiscard]] ultra::value_t eval() const override
    {
      return ultra::D_INT(0);
    }
  };
}  // namespace

TEST_SUITE("value_t formatter")
{

TEST_CASE("Formats void as braces")
{
  ultra::value_t v = ultra::D_VOID();
  CHECK(std::format("{}", v) == "{}");
}

TEST_CASE("Formats int and double")
{
  CHECK(std::format("{}", ultra::value_t(ultra::D_INT(42))) == "42");

  // Keep it robust: don't over-specify floating rendering rules.
  const auto s(std::format("{}", ultra::value_t(ultra::D_DOUBLE(3.5))));
  CHECK(s.find("3.5") == 0);
}

TEST_CASE("Formats string with quotes and escaping")
{
  CHECK(std::format("{}", ultra::value_t(ultra::D_STRING("abc")))
        == "\"abc\"");

  // Escapes " and \ (same intent as std::quoted for these two)
  CHECK(std::format("{}", ultra::value_t(ultra::D_STRING("a\"b")))
        == "\"a\\\"b\"");
  CHECK(std::format("{}", ultra::value_t(ultra::D_STRING("a\\b")))
        == "\"a\\\\b\"");
}

TEST_CASE("Formats address as bracketed underlying integer")
{
  using ultra::operator""_addr;
  CHECK(std::format("{}", ultra::value_t(123_addr)) == "[123]");
}

TEST_CASE("Formats ivector as brace list with spaces")
{
  CHECK(std::format("{}", ultra::value_t(ultra::D_IVECTOR())) == "{}");
  CHECK(std::format("{}", ultra::value_t(ultra::D_IVECTOR{1})) == "{1}");
  CHECK(std::format("{}", ultra::value_t(ultra::D_IVECTOR{1, 2, 3}))
        == "{1 2 3}");
}

TEST_CASE("Formats nullary pointer via to_string")
{
  const test_nullary n("f");
  ultra::value_t v(static_cast<const ultra::D_NULLARY *>(&n));
  CHECK(std::format("{}", v) == "f()");
}

TEST_CASE("Formats variable pointer via to_string")
{
  // Variable ctor: (var_id, name, category)
  ultra::src::variable x(0u, "x");
  ultra::value_t v(static_cast<const ultra::D_VARIABLE *>(&x));
  CHECK(std::format("{}", v) == "x");
}

TEST_CASE("null pointer fallbacks")
{
  ultra::value_t vn(static_cast<const ultra::D_NULLARY *>(nullptr));
  ultra::value_t vv(static_cast<const ultra::D_VARIABLE *>(nullptr));

  CHECK(std::format("{}", vn) == "<nullary:null>");
  CHECK(std::format("{}", vv) == "<var:null>");
}

}  // TEST_SUITE
