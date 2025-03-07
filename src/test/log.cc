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

#include <filesystem>

#include "utility/log.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

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

  const auto logpath(log::setup_stream("debug"));
  std::string line, msg;

  log::reporting_level = log::lOFF;
  msg = "Fatal message";
  ultraFATAL << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lFATAL;
  ultraFATAL << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "Error message";
  ultraERROR << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lERROR;
  ultraERROR << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "Warning message";
  ultraWARNING << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lWARNING;
  ultraWARNING << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(line.find(msg) != std::string::npos);
  }

  msg = "ParOut message";
  ultraPAROUT << msg;
  {
    std::ifstream logstream(logpath);
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(std::getline(logstream, line));
    CHECK(!std::getline(logstream, line));
  }

  log::reporting_level = log::lPAROUT;
  ultraPAROUT << msg;
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

  msg = "Debug message";
  ultraDEBUG << msg;
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
}

}  // TEST_SUITE("LOG")
