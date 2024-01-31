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

#include <numbers>

#include "kernel/gp/function.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("MISC")
{

TEST_CASE("issmall")
{
  using namespace ultra;

  const double a(1.0);
  const double ae(a + std::numeric_limits<double>::epsilon());
  const double a2e(a + 2.0 * std::numeric_limits<double>::epsilon());

  CHECK(issmall(a - ae));
  CHECK(issmall(ae - a));
  CHECK(!issmall(a - a2e));
  CHECK(!issmall(a2e - a));
  CHECK(!issmall(0.1));
}

TEST_CASE("isnonnegative")
{
  using namespace ultra;

  CHECK(isnonnegative(0));
  CHECK(isnonnegative(0.0));
  CHECK(isnonnegative(1));
  CHECK(isnonnegative(0.000001));
  CHECK(!isnonnegative(-1));
  CHECK(!isnonnegative(-0.00001));
}

TEST_CASE("lexical_cast")
{
  using namespace ultra;

  CHECK(lexical_cast<double>(std::string("2.5")) == doctest::Approx(2.5));
  CHECK(lexical_cast<int>(std::string("2.5")) == 2);
  CHECK(lexical_cast<std::string>(std::string("abc")) == "abc");

  CHECK(lexical_cast<double>(value_t()) == doctest::Approx(0.0));
  CHECK(lexical_cast<double>(value_t(2.5)) == doctest::Approx(2.5));
  CHECK(lexical_cast<double>(value_t(2)) == doctest::Approx(2));
  CHECK(lexical_cast<double>(value_t("3.1")) == doctest::Approx(3.1));

  CHECK(lexical_cast<int>(value_t()) == 0);
  CHECK(lexical_cast<int>(value_t(2.5)) == 2);
  CHECK(lexical_cast<int>(value_t(2)) == 2);
  CHECK(lexical_cast<int>(value_t("3.1")) == 3);

  CHECK(lexical_cast<std::string>(value_t()) == "");
  CHECK(std::stod(lexical_cast<std::string>(value_t(2.5)))
        == doctest::Approx(2.5));
  CHECK(lexical_cast<std::string>(value_t(2)) == "2");
  CHECK(lexical_cast<std::string>(value_t("abc")) == "abc");
}

TEST_CASE("almost_equal")
{
  using namespace ultra;

  CHECK(almost_equal(2.51, 2.51000001));
  CHECK(!almost_equal(2.51, 2.511));
}

TEST_CASE("save/load float to/from stream")
{
  using namespace ultra;

  std::stringstream ss;
  save_float_to_stream(ss, 2.5);

  double d;
  load_float_from_stream(ss, &d);
  CHECK(d  == doctest::Approx(2.5));
}

TEST_CASE("as_integer")
{
  using namespace ultra;

  enum class my_enum {a = 3, b, c};

  CHECK(as_integer(my_enum::a) == 3);
  CHECK(as_integer(my_enum::b) == 4);
  CHECK(as_integer(my_enum::c) == 5);
}

TEST_CASE("is_number")
{
  using namespace ultra;

  CHECK(is_number("3.1"));
  CHECK(is_number("3"));
  CHECK(is_number("   3 "));
  CHECK(!is_number("aa3aa"));
  CHECK(!is_number(""));
  CHECK(!is_number("abc"));
}

TEST_CASE("iequals")
{
  using namespace ultra;

  CHECK(iequals("abc", "ABC"));
  CHECK(iequals("abc", "abc"));
  CHECK(iequals("ABC", "ABC"));
  CHECK(!iequals("ABC", " ABC"));
  CHECK(!iequals("ABC", "AB"));
  CHECK(!iequals("ABC", ""));
}

TEST_CASE("trim")
{
  using namespace ultra;

  CHECK(trim("abc") == "abc");
  CHECK(trim("  abc") == "abc");
  CHECK(trim("abc  ") == "abc");
  CHECK(trim("  abc  ") == "abc");
  CHECK(trim("") == "");
}

TEST_CASE("replace")
{
  using namespace ultra;

  CHECK(replace("suburban", "sub", "") == "urban");
  CHECK(replace("  cde", "  ", "ab") == "abcde");
  CHECK(replace("abcabc", "abc", "123") == "123abc");
  CHECK(replace("abc", "bcd", "") == "abc");
  CHECK(replace("", "a", "b") == "");
}

TEST_CASE("replace_all")
{
  using namespace ultra;

  CHECK(replace_all("suburban", "sub", "") == "urban");
  CHECK(replace_all("abcabc", "abc", "123") == "123123");
  CHECK(replace_all("abcdabcdabcdabcd", "cd", "") == "abababab");
}

TEST_CASE("iterator_of")
{
  using namespace ultra;

  std::vector v = {1, 2, 3, 4, 5};
  std::vector v1 = {6, 7, 8};

  CHECK(iterator_of(std::next(v.begin(), 2), v));
  CHECK(!iterator_of(v1.begin(), v));
}

}  // TEST_SUITE("BASE")
