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
#include <memory>
#include <mutex>
#include <print>

namespace
{

std::unique_ptr<std::ostream> stream_ {nullptr};  // long term log stream
std::mutex stream_mutex_;  // protects `stream_` writes

}  // namespace


namespace ultra
{

namespace internal
{

[[nodiscard]] std::mutex &console_mutex() noexcept
{
  static std::mutex m;
  return m;
}

void emit(log::level l, std::string message)
{
  l = std::min(log::lOFF, l);
  const auto now(std::chrono::system_clock::now());

  {
    std::lock_guard lock(stream_mutex_);

    if (stream_)  // `stream_`, if available, gets all the messages
    {
      // Append '\n' without forcing a flush (avoid `std::endl`).
      (*stream_) << std::format("{:%F %T}\t{}\t{}", now, l, message) << '\n';
    }
  }

  if (l >= log::reporting_level)  // `stdout` is selective
  {
    std::lock_guard lock(internal::console_mutex());

    // Clear the line using a width specifier and a carriage return.
    std::print("\r{:70}\r", "");

    // Print the tag if necessary.
    if (l != log::level::lSTDOUT && l != log::level::lPAROUT)
      std::print("[{}] ", l);

    // Flush for console.
    std::println("{}", message);
  }
}

}  // namespace internal

namespace log
{

level reporting_level {lINFO};

void flush()
{
  std::lock_guard lock(stream_mutex_);
  if (stream_)
    stream_->flush();
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
std::filesystem::path setup_stream(const std::string &base)
{
  using namespace std::chrono;

  const auto now(floor<seconds>(system_clock::now()));
  const std::filesystem::path fp(
    std::format("{}_{:%j_%H_%M_%S}.log", base, now));

  if (auto new_stream(std::make_unique<std::ofstream>(fp)); *new_stream)
  {
    std::lock_guard lock(stream_mutex_);
    stream_ = std::move(new_stream);
    return fp;
  }

  return {};
}

}  // namespace log

}  // namespace ultra
