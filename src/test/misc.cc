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
#include <thread>

#include "kernel/gp/function.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

enum class myenum : unsigned
{
  disabled = 0, A = 1, B = 2, C = 4, all = 7
};

/// Enable bitmask operations for `init`.
template<> struct ultra::is_bitmask_enum<myenum> : std::true_type {};

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

  using namespace std::chrono_literals;
  CHECK(lexical_cast<std::string>(2ms) == "0.002");
  CHECK(lexical_cast<std::string>(1s) == "1.000");
  CHECK(lexical_cast<std::string>(12min) == "12:00");
  CHECK(lexical_cast<std::string>(1h) == "01:00:00");
  CHECK(lexical_cast<std::string>(26h) == "1:02:00:00");
}

TEST_CASE("almost_equal")
{
  using namespace ultra;

  CHECK(almost_equal(2.51, 2.51000001));
  CHECK(!almost_equal(2.51, 2.511));
  CHECK(almost_equal(std::numeric_limits<double>::infinity(),
                     std::numeric_limits<double>::infinity()));
  CHECK(!almost_equal(std::numeric_limits<double>::infinity(),
                      -std::numeric_limits<double>::infinity()));
  CHECK(!almost_equal(std::numeric_limits<double>::quiet_NaN(),
                      std::numeric_limits<double>::quiet_NaN()));
  CHECK(almost_equal(std::numeric_limits<double>::min(),
                     std::numeric_limits<double>::min()));
  CHECK(almost_equal(std::numeric_limits<double>::lowest(),
                     std::numeric_limits<double>::lowest()));
  CHECK(almost_equal(std::numeric_limits<double>::max(),
                     std::numeric_limits<double>::max()));
  CHECK(almost_equal(std::numeric_limits<double>::epsilon(),
                     std::numeric_limits<double>::epsilon()));
  CHECK(almost_equal(std::numeric_limits<double>::denorm_min(),
                     std::numeric_limits<double>::denorm_min()));
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

TEST_CASE("is_integer")
{
  using namespace ultra;

  CHECK(is_integer("3"));
  CHECK(is_integer("   3 "));
  CHECK(is_integer("+3"));
  CHECK(is_integer("-3"));
  CHECK(!is_integer(""));
  CHECK(!is_integer("aa3aa"));
  CHECK(!is_integer("abc"));
  CHECK(!is_integer("3.1"));
}

TEST_CASE("is_number")
{
  using namespace ultra;

  CHECK(is_number("3.1"));
  CHECK(is_number("3"));
  CHECK(is_number("   3 "));
  CHECK(is_number("+3"));
  CHECK(is_number("-3"));
  CHECK(!is_number("inf"));
  CHECK(!is_number("+inf"));
  CHECK(!is_number("-inf"));
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

  const std::vector v = {1, 2, 3, 4, 5};
  const std::vector v1 = {6, 7, 8};

  CHECK(iterator_of(std::next(v.begin(), 2), v));
  CHECK(!iterator_of(v1.begin(), v));
}

TEST_CASE("get_index")
{
  using namespace ultra;

  const std::vector v = {1, 2, 3, 4, 5, 6, 7, 8};

  for (std::size_t i(0); i < v.size(); ++i)
    CHECK(get_index(v[i], v) == i);
}

TEST_CASE("app_level_uid")
{
  using namespace ultra;

  const app_level_uid id1;
  const app_level_uid id2;

  CHECK(id1 + 1 == id2);
}

TEST_CASE("Base CRC32")
{
  CHECK(ultra::crc32::calculate("The quick brown fox jumps over the lazy dog")
        == 0x414FA339);
  CHECK(ultra::crc32::calculate("123456789") == 0xcbf43926);
  CHECK(ultra::crc32::calculate("") == 0);

  const std::string empty_xml(R"(<?xml version="1.0"?>
<checksum>00000000</checksum>)");

  const std::string signed_xml(ultra::crc32::embed_xml_signature(empty_xml));
  CHECK(signed_xml != empty_xml);
  CHECK(ultra::crc32::verify_xml_signature(signed_xml));
}

TEST_CASE("CRC32 with parallel processes")
{
  using namespace ultra;
  namespace fs = std::filesystem;

  const std::string base_xml(R"(<?xml version="1.0"?>
<customers>
   <customer id="55000">
      <name>Charter Group</name>
      <address>
         <street>100 Main</street>
         <city>Framingham</city>
         <state>MA</state>
         <zip>01701</zip>
      </address>
      <address>
         <street>720 Prospect</street>
         <city>Framingham</city>
         <state>MA</state>
         <zip>01701</zip>
      </address>
      <address>
         <street>120 Ridge</street>
         <state>MA</state>
         <zip>01760</zip>
      </address>
   </customer>
   <checksum>00000000</checksum>
</customers>)");

  const std::string xml(crc32::embed_xml_signature(base_xml));

  const fs::path data_file("data.xml");

  const auto cleanup([&]()
  {
    if (fs::exists(data_file))
      fs::remove(data_file);
  });

  const auto reader([&]()
  {
    unsigned reads(100);

    while (reads)
      if (std::ifstream file(data_file); file)
      {
        const std::string data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

        if (crc32::verify_xml_signature(data))
        {
          CHECK(data == xml);
          --reads;
        }
      }
  });

  const auto writer([&]()
  {
    unsigned writes(10);

    while (writes)
    {
      // Write to the file.
      std::ofstream file(data_file);
      CHECK(file);
      file << xml;

      --writes;
      std::this_thread::sleep_for(50ms);
    }
  });

  cleanup();

  {
    std::ofstream file(data_file);
    CHECK(file);
    file << xml;
  }

  std::thread read_thread(reader);
  writer();

  read_thread.join();

  CHECK(fs::exists(data_file));

  cleanup();
}

TEST_CASE("Bitmask enum")
{
  using namespace ultra;

  myenum off(myenum::disabled);
  myenum a(myenum::A), b(myenum::B), c(myenum::C);
  myenum all(a|b|c);

  CHECK(!has_flag(off, myenum::A));
  CHECK(!has_flag(off, myenum::B));
  CHECK(!has_flag(off, myenum::C));
  CHECK(!has_flag(off, myenum::all));

  CHECK(has_flag(all, myenum::A));
  CHECK(has_flag(all, myenum::B));
  CHECK(has_flag(all, myenum::C));
  CHECK(has_flag(all, myenum::all));
  CHECK(as_integer(all & a));
  CHECK(as_integer(all & b));
  CHECK(as_integer(all & c));

  CHECK(!has_flag(all ^ a, a));
  CHECK(!has_flag(all ^ b, b));
  CHECK(!has_flag(all ^ c, c));
  CHECK(has_flag(all ^ a, b));
  CHECK(has_flag(all ^ a, c));
  CHECK(has_flag(all ^ b, a));
  CHECK(has_flag(all ^ b, c));
  CHECK(has_flag(all ^ c, a));
  CHECK(has_flag(all ^ c, b));

  CHECK(has_flag(a, myenum::A));
  CHECK(!has_flag(a, myenum::B));
  CHECK(!has_flag(a, myenum::C));

  CHECK(!has_flag(b, myenum::A));
  CHECK(has_flag(b, myenum::B));
  CHECK(!has_flag(b, myenum::C));

  CHECK(!has_flag(c, myenum::A));
  CHECK(!has_flag(c, myenum::B));
  CHECK(has_flag(c, myenum::C));
}

}  // TEST_SUITE("MISC")
