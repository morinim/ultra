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

TEST_CASE("File locking mechanism")
{
  using namespace ultra;
  namespace fs = std::filesystem;

  const std::string initial_content("Initial content");
  const std::string updated_content("Updated content");

  const fs::path main_file("data.txt");
  const fs::path read_lock_file("data.read.lock");
  const fs::path write_lock_file("data.write.lock");

  const auto cleanup([&]()
  {
    if (fs::exists(write_lock_file))
      fs::remove(write_lock_file);
    if (fs::exists(read_lock_file))
      fs::remove(read_lock_file);
    if (fs::exists(main_file))
      fs::remove(main_file);
  });

  const auto reader([&]
  {
    unsigned reads(100);

    while (reads)
      if (lock_file::acquire_read(main_file))
      {
        // Read the file.
        std::ifstream file(main_file);
        CHECK(file);

        std::string line;
        while (std::getline(file, line))
        {
          const bool ok(line == initial_content || line == updated_content);
          CHECK(ok);
        }

        lock_file::release_read(main_file);

        --reads;
      }
  });

  const auto writer([&]
  {
    unsigned writes(10);

    while (writes)
    {
      lock_file::acquire_write(main_file);

      // Write to the file.
      std::ofstream file(main_file);
      CHECK(file);
      file << updated_content << '\n';

      lock_file::release_write(main_file);

      --writes;
      std::this_thread::sleep_for(50ms);
    }
  });

  cleanup();

  {
    std::ofstream file(main_file);
    CHECK(file);
    file << initial_content << '\n';
  }

  // Use multiple threads to simulate multiple readers and a single writer
  // accessing the shared resource concurrently.
  std::vector<std::jthread> threads;

  for (unsigned i(0); i < 4; ++i)
    threads.emplace_back(reader);

  //threads.emplace_back(writer);
  writer();

  for (auto &thread : threads)
    if (thread.joinable())
      thread.join();

  CHECK(fs::exists(main_file));
  CHECK(!fs::exists(read_lock_file));
  CHECK(!fs::exists(write_lock_file));

  cleanup();

}

}  // TEST_SUITE("MISC")
