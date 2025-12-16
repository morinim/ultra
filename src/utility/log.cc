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
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "utility/log.h"

namespace ultra
{

log::level log::reporting_level {log::lINFO};
std::unique_ptr<std::ostream> log::stream_ {nullptr};
std::mutex log::emit_mutex_;
thread_local std::ostringstream log::tls_buffer_;


///
/// Sets the logging level of a message.
///
/// \param[in] l level associated to the streamed message
///
/// The following code:
///
///     log().get(level) << "Hello " << username;
///
/// creates a `log` object with the `level` logging level (everything at with
/// a level lower than `level` is ignored), fetches its `std::stringstream`
/// object, formats and accumulates the user-supplied data and, finally:
/// - prints the resulting string on `std::cout`;
/// - persists the resulting string into the log file (if specified).
///
std::ostringstream &log::get(level l)
{
  level_ = std::min(lOFF, l);
  tls_buffer_.str({});
  tls_buffer_.clear();
  return tls_buffer_;
}

log::~log()
{
  static const std::string tags[] =
  {
    "DEBUG", "INFO", "", "", "WARNING", "ERROR", "FATAL", ""
  };

  const auto message(tls_buffer_.str());

  std::lock_guard lock(emit_mutex_);

  if (stream_)  // `stream_`, if available, gets all the messages
  {
    const auto now(std::chrono::system_clock::now());
    const std::time_t t_c(std::chrono::system_clock::to_time_t(now));

    // Do not flush (`\n`) for generic streams.
    // Also remember the `localtime` is not thread-safe on POSIX (returns a
    // pointer to static storage). It's safe here because we hold
    // `emit_mutex_`.
    *stream_ << std::put_time(std::localtime(&t_c), "%F %T")
             << '\t' << tags[level_] << '\t' << message << '\n';
  }

  if (level_ >= reporting_level)  // `cout` is selective
  {
    const std::string clear_line(std::string(60, ' ') + std::string(1, '\r'));
    std::cout << clear_line;

    if (level_ != lSTDOUT && level_ != lPAROUT)
      std::cout << '[' << tags[level_] << "] ";

    // Flush (`std::endl`) for console.
    std::cout << message << std::endl;
  }
}

void log::flush()
{
  if (stream_)
  {
    std::lock_guard lock(emit_mutex_);
    stream_->flush();
  }
}

///
/// Sets the output stream for logging.
///
/// \param[in] base base filepath of the log (e.g. `/home/doe/app`)
///
/// Given the `/home/doe/app` argument associates the `log::stream` variable
/// with the `app_123_18_30_00.log` file (the numbers represents the current:
/// day of the year, hours, minutes, seconds) in the `/home/doe/` directory.
///
std::filesystem::path log::setup_stream(const std::string &base)
{
  using namespace std::chrono;

  const std::time_t t_c(system_clock::to_time_t(system_clock::now()));

  std::ostringstream fn;
  fn << base << std::put_time(std::localtime(&t_c), "_%j_%H_%M_%S") << ".log";
  const std::filesystem::path fp(fn.str());

  stream_ = std::make_unique<std::ofstream>(fp);
  return fp;
}

}  // namespace ultra
