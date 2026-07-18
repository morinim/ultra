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
#include <format>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace ultra
{

///
/// A basic console printer with integrated logger.
///
/// This API is re-entrant because message construction happens in a local
/// `std::string` via `std::format`.
///
namespace log
{

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
extern level reporting_level;

/// Returns whether a level passes the runtime reporting threshold.
///
/// This function deliberately does not apply `NDEBUG` filtering. The logging
/// macros perform that filtering in the consuming translation unit so the
/// behaviour follows the consumer's build configuration.
[[nodiscard]] inline bool enabled(level l) noexcept
{
  return l >= reporting_level;
}

std::filesystem::path setup_stream(const std::string & = "ultra");

void flush();

}  // namespace log

namespace internal
{

template<class T>
[[nodiscard]] std::string streamed(const T &value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

#if defined(NDEBUG)
[[maybe_unused]] static constexpr bool debug_logging_enabled {false};
#else
[[maybe_unused]] static constexpr bool debug_logging_enabled {true};
#endif

extern void emit(log::level, std::string);

}  // namespace internal

namespace log
{

template<class... Args>
static void print(level l, std::format_string<Args...> fmt, Args &&... args)
{
  internal::emit(l, std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace log

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
///     ultraPRINT(log::lINFO, "A number of {} were notified.",
///                NotifyClients());
///
/// the clients will be notified only if the logging level will be `log::lINFO`
/// and greater.
///
/// \note
/// When the `NDEBUG` is defined all the debug-level logging is eliminated at
/// compile time.
///
#define ultraPRINT(level, ...)                                  \
  do                                                            \
  {                                                             \
    const auto ultra_detail_log_level_(level);                  \
    if ((ultra::internal::debug_logging_enabled                 \
         || ultra_detail_log_level_ != ultra::log::lDEBUG)      \
        && ultra::log::enabled(ultra_detail_log_level_))        \
      ultra::log::print(ultra_detail_log_level_, __VA_ARGS__);  \
  } while (false)

#define ultraDEBUG(...)   \
  ultraPRINT(ultra::log::lDEBUG, __VA_ARGS__)
#define ultraINFO(...)    \
  ultraPRINT(ultra::log::lINFO, __VA_ARGS__)
#define ultraSTDOUT(...)  \
  ultraPRINT(ultra::log::lSTDOUT, __VA_ARGS__)
#define ultraPAROUT(...)  \
  ultraPRINT(ultra::log::lPAROUT, __VA_ARGS__)
#define ultraWARNING(...) \
  ultraPRINT(ultra::log::lWARNING, __VA_ARGS__)
#define ultraERROR(...)   \
  ultraPRINT(ultra::log::lERROR, __VA_ARGS__)
#define ultraFATAL(...)   \
  ultraPRINT(ultra::log::lFATAL, __VA_ARGS__)

}  // namespace ultra

template<>
struct std::formatter<ultra::log::level, char>
  : std::formatter<std::string_view, char>
{
  template<class FormatContext>
  [[nodiscard]] auto format(ultra::log::level l, FormatContext &ctx) const
  {
    std::string_view name;

    switch (l)
    {
    case ultra::log::lDEBUG:   name =   "DEBUG"; break;
    case ultra::log::lINFO:    name =    "INFO"; break;
    case ultra::log::lWARNING: name = "WARNING"; break;
    case ultra::log::lERROR:   name =   "ERROR"; break;
    case ultra::log::lFATAL:   name =   "FATAL"; break;

    case ultra::log::lSTDOUT:
    case ultra::log::lPAROUT:
    case ultra::log::lOFF:
      break;
    }

    return std::formatter<std::string_view, char>::format(name, ctx);
  }
};

static_assert(std::formattable<ultra::log::level, char>);

#endif  // include guard
