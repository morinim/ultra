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

#include "utility/log.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <print>
#include <string_view>

auto std::formatter<ultra::log::level>::format(ultra::log::level l,
                                               format_context &ctx) const
{
  std::string_view name("");

  switch (l)
  {
  case ultra::log::lDEBUG:   name =   "DEBUG"; break;
  case ultra::log::lINFO:    name =    "INFO"; break;
  case ultra::log::lWARNING: name = "WARNING"; break;
  case ultra::log::lERROR:   name =   "ERROR"; break;
  case ultra::log::lFATAL:   name =   "FATAL"; break;
  default:  break;  // explicitly do nothing for lSTDOUT/lPAROUT
  }

  return std::formatter<std::string_view>::format(name, ctx);
}

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
/// - prints the resulting string on `stdout`;
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
  const auto message(tls_buffer_.str());
  const auto now(std::chrono::system_clock::now());

  std::lock_guard lock(emit_mutex_);

  if (stream_)  // `stream_`, if available, gets all the messages
  {
    // Append '\n' without forcing a flush (avoid `std::endl`).
    (*stream_) << std::format("{:%F %T}\t{}\t{}", now, level_, message)
               << '\n';
  }

  if (level_ >= reporting_level)  // `stdout` is selective
  {
    // Clear the line using a width specifier and a carriage return.
    std::print("\r{:60}\r", "");

    // Print the tag if necessary.
    if (level_ != lSTDOUT && level_ != lPAROUT)
      std::print("[{}] ", level_);

    // Flush for console.
    std::println("{}", message);
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
/// Sets (or replaces) the persistent log output stream.
///
/// \param[in] base base filepath used to build the log filename
///                 (e.g. `/home/doe/app`)
/// \return         the full path of the log file actually opened on success;
///                 an empty path (`{}`) on failure
///
/// Given the `/home/doe/app` argument associates the `log::stream` variable
/// with the `app_123_18_30_00.log` file (the numbers represents the current:
/// day of the year, hours, minutes, seconds) in the `/home/doe/` directory.
/// The log file path is obtained by appending a timestamp suffix and the
/// `.log` extension to `base`.
///
/// If the file can be opened, the internal stream (`log::stream_`) is
/// replaced atomically with the newly created `std::ofstream`. The update is
/// protected so that concurrent emissions cannot interleave with the stream
/// replacement.
///
/// On failure (e.g. invalid path, missing permissions, non-existent
/// directory), the current logging stream is left unchanged and an empty path
/// is returned.
///
/// \note
/// This function only affects persistence to the log file. Console output
/// continues to be controlled by `log::reporting_level`.
///
std::filesystem::path log::setup_stream(const std::string &base)
{
  using namespace std::chrono;

  const auto now(floor<seconds>(system_clock::now()));
  const std::filesystem::path fp(
    std::format("{}_{:%j_%H_%M_%S}.log", base, now));

  if (auto new_stream(std::make_unique<std::ofstream>(fp)); *new_stream)
  {
    std::lock_guard lock(emit_mutex_);
    stream_ = std::move(new_stream);
    return fp;
  }

  return {};
}

}  // namespace ultra
