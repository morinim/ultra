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

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "utility/log.h"

namespace ultra
{

log::level log::reporting_level = log::lINFO;
std::unique_ptr<std::ostream> log::stream = nullptr;

///
/// Sets the logging level of a message.
///
/// \param[in] l logging level of the following message
///
/// The following code:
///
///     log().get(level) << "Hello " << username;
///
/// creates a `log` object with the `level` logging level, fetches its
/// `std::stringstream` object, formats and accumulates the user-supplied data
/// and, finally:
/// - prints the resulting string on `std::cout`;
/// - persists the resulting string into the log file (if specified).
///
std::ostringstream &log::get(level l)
{
  level_ = std::min(lFATAL, l);
  return os;
}

log::~log()
{
  static const std::string tags[] =
  {
    "ALL", "DEBUG", "INFO", "", "WARNING", "ERROR", "FATAL", ""
  };

  if (stream)  // `stream`, if available, gets all the messages
  {
    using namespace std::chrono;
    const std::time_t t_c(system_clock::to_time_t(system_clock::now()));

    std::osyncstream(*stream) << std::put_time(std::localtime(&t_c), "%F %T")
                              << '\t' << tags[level_]
                              << '\t' << os.str() << std::endl;
  }

  if (level_ >= reporting_level)  // `cout` is selective
  {
    std::string tag;
    if (level_ != lOUTPUT)
      tag = "[" + tags[level_] + "] ";

    std::osyncstream(std::cout) << tag << os.str() << std::endl;
  }
}

///
/// Sets the `log::stream` variable with a convenient value.
///
/// \param[in] base base filepath of the log (e.g. `/home/doe/app`)
///
/// Given the `/home/doe/app` argument associates the `log::stream` variable
/// with the `app_123_18_30_00.log` file (the numbers represents the current:
/// day of the year, hours, minutes, seconds) in the `/home/doe/` directory.
///
void log::setup_stream(const std::string &base)
{
  using namespace std::chrono;
  const std::time_t t_c(system_clock::to_time_t(system_clock::now()));

  std::ostringstream fn;
  fn << base << std::put_time(std::localtime(&t_c), "_%j_%H_%M_%S") << ".log";

  stream = std::make_unique<std::ofstream>(fn.str());
}

}  // namespace ultra
