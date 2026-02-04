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

#if !defined(ULTRA_LOG_H)
#define      ULTRA_LOG_H

#include <filesystem>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

namespace ultra
{

///
/// A basic console printer with integrated logger.
///
/// \note
/// This is derived from the code presented in "Logging in C++" by Petru
/// Marginean (DDJ Sep 2007)
///
class log final
{
public:
  /// The log level.
  ///
  /// * `lDEBUG`   - Only interesting for developers
  /// * `lINFO`    - I say something but I don't expect you to listen
  /// * `lSTDOUT`  - Standard console output
  /// * `lPAROUT`  - Console with multiple concurrent linked searches
  /// * `lWARNING` - I can continue but please have a look
  /// * `lERROR`   - Something really wrong... but you could be lucky
  /// * `lFATAL`   - The program cannot continue
  /// * `lOFF`     - Disable output
  ///
  /// \remarks
  /// The `lDEBUG` log level is available only in non-`NDEBUG` builds.
  enum level {lDEBUG, lINFO, lSTDOUT, lPAROUT, lWARNING, lERROR, lFATAL, lOFF};

  /// Current reporting level: messages with a lower level aren't logged /
  /// printed.
  static level reporting_level;

  static std::filesystem::path setup_stream(const std::string & = "ultra");

  static void flush();

  log() = default;
  log(const log &) = delete;
  log &operator=(const log &) = delete;

  ~log();

  [[nodiscard]] std::ostringstream &get(level = lSTDOUT);

private:
  // Thread-local buffer for message construction.
  static thread_local std::ostringstream tls_buffer_;

  static std::unique_ptr<std::ostream> stream_;  // long term log stream
  static std::mutex emit_mutex_;                 // protects the final emission

  level level_ {lSTDOUT};  // current log level
};

///
/// A little trick that makes the code, when logging is not necessary, almost
/// as fast as the code with no logging at all.
///
/// Logging will have a cost only if it actually produces output; otherwise,
/// the cost is low (and immeasurable in most cases). This lets you control the
/// trade-off between fast execution and detailed logging.
///
/// Macro-related dangers should be avoided: we shouldn't forget that the
/// logging code might not be executed at all, subject to the logging level in
/// effect. This is what we actually wanted and is actually what makes the code
/// efficient. But as always, "macro-itis" can introduce subtle bugs. In this
/// example:
///
///     ultraPRINT(log::lINFO) << "A number of " << NotifyClients()
///                            << " were notified.";
///
/// the clients will be notified only if the logging level will will be
/// `log::lINFO` and greater. Probably not what was intended! The correct code
/// should be:
///
///     const int notifiedClients = NotifyClients();
///     ultraPRINT(log::lINFO) << "A number of " << notifiedClients
///                            << " were notified.";
///
///
/// \note
/// When the `NDEBUG` is defined all the debug-level logging is eliminated at
/// compile time.
#if defined(NDEBUG)
#define ultraPRINT(level) if ((level) == log::lDEBUG);               \
                          else if ((level) < log::reporting_level);  \
                          else ultra::log().get(level)
#else
#define ultraPRINT(level) if ((level) < log::reporting_level);  \
                          else ultra::log().get(level)
#endif

#define ultraDEBUG   ultraPRINT(log::lDEBUG)
#define ultraERROR   ultraPRINT(log::lERROR)
#define ultraFATAL   ultraPRINT(log::lFATAL)
#define ultraINFO    ultraPRINT(log::lINFO)
#define ultraPAROUT  ultraPRINT(log::lPAROUT)
#define ultraSTDOUT  ultraPRINT(log::lSTDOUT)
#define ultraWARNING ultraPRINT(log::lWARNING)

}  // namespace ultra

#endif  // include guard
