/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "utility/log.h"

#include <filesystem>
#include <fstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace
{

class scoped_file_cleanup
{
public:
  explicit scoped_file_cleanup(const std::filesystem::path &p) : p_(p) {}

  ~scoped_file_cleanup()
  {
    [[maybe_unused]] std::error_code ec;
    std::filesystem::remove(p_, ec);  // best-effort cleanup
  }

private:
  std::filesystem::path p_;
};

}  // namespace


TEST_SUITE("LOG")
{

TEST_CASE("Order of levels")
{
  using namespace ultra;

  static_assert(log::lDEBUG < log::lINFO);
  static_assert(log::lINFO < log::lSTDOUT);
  static_assert(log::lSTDOUT < log::lPAROUT);
  static_assert(log::lPAROUT < log::lWARNING);
  static_assert(log::lWARNING < log::lERROR);
  static_assert(log::lERROR < log::lFATAL);
  static_assert(log::lFATAL < log::lOFF);
}

TEST_CASE("Reporting level")
{
  using namespace ultra;

  // `setup_stream` creates a temporary file.
  const auto logpath(log::setup_stream((std::filesystem::temp_directory_path()
                                        / "debug").string()));
  REQUIRE(!logpath.empty());

  scoped_file_cleanup cleanup(logpath);

  std::string line, msg;

  log::reporting_level = log::lOFF;
  msg = "Fatal message";
  ultraFATAL << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lFATAL;
  ultraFATAL << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "Error message";
  ultraERROR << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lERROR;
  ultraERROR << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "Warning message";
  ultraWARNING << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lWARNING;
  ultraWARNING << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "ParOut message";
  ultraPAROUT << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lPAROUT;
  ultraPAROUT << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "StdOut message";
  ultraSTDOUT << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lSTDOUT;
  ultraSTDOUT << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "Info message";
  ultraINFO << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lINFO;
  ultraINFO << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

#if !defined(NDEBUG)
  msg = "Debug message";
  ultraDEBUG << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lDEBUG;
  ultraDEBUG << msg;
  log::flush();
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }
#endif

}

}  // TEST_SUITE("LOG")
