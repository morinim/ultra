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

#include "utility/term.h"
#include "utility/log.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
// The `windows.h` header file (or more correctly, `windef.h` that it includes
// in turn) has macros for `min` and `max` that are interfering with
// `std::min`/`max` and `std::numeric_limits<T>::min`/`max`.
// The `NOMINMAX` macro suppresses the `min` and `max` definitions in
// `Windef.h`. The `undef` limits potential side effects.
#  define NOMINMAX
#  include <conio.h>
#  include <windows.h>
#  undef NOMINMAX
#else
#  include <iostream>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace ultra::term
{

#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32)
///
/// \param[in] enter if `true` sets the terminal raw mode, else restore the
///                  default terminal mode
///
/// The raw mode discipline performs no line editing and the control
/// sequences for both line editing functions and the various special
/// characters ("interrupt", "quit", and flow control) are treated as normal
/// character input. Applications programs reading from the terminal receive
/// characters immediately and receive the entire character stream unaltered,
/// just as it came from the terminal device itself.
///
void term_raw_mode(bool enter)
{
  static termios oldt, newt;

  if (enter)
  {
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~static_cast<unsigned>(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

///
/// \return `true` if the user press a key (`false` otherwise)
///
[[nodiscard]] bool kbhit()
{
  // Do not wait at all, not even a microsecond.
  timeval tv;
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  fd_set readfd;
  FD_ZERO(&readfd);  // initialize `readfd`
  FD_SET(STDIN_FILENO, &readfd);

  // The first parameter is the number of the largest file descriptor to
  // check + 1.
  if (select(STDIN_FILENO + 1, &readfd, nullptr, nullptr, &tv) == -1)
    return false;  // an error occurred
  // `read_fd` now holds a bit map of files that are readable. We test the
  // entry for the standard input (file 0).
  return FD_ISSET(STDIN_FILENO, &readfd);
}

[[nodiscard]] bool keypressed(int k) { return kbhit() && std::cin.get() == k; }

#else

void term_raw_mode(bool) {}
[[nodiscard]] bool keypressed(int k) { return _kbhit() && _getch() == k; }

#endif

///
/// \return `true` when the user presses the '.' key
///
bool user_stop()
{
  const bool stop(keypressed('.'));

  if (stop)
  {
    ultraINFO << "Stopping evolution...";
  }

  return stop;
}

///
/// Resets the term and restores the default signal handlers.
///
void reset()
{
  std::signal(SIGABRT, SIG_DFL);
  std::signal(SIGINT, SIG_DFL);
  std::signal(SIGTERM, SIG_DFL);

  term_raw_mode(false);
}

///
/// If the program receives a `SIGABRT` / `SIGINT` / `SIGTERM`, it must handle
/// the signal and reset the terminal to the initial state.
///
void signal_handler(int signum)
{
  term::reset();

  std::raise(signum);
}

///
/// Sets the term in raw mode and handles the interrupt signals.
///
void set()
{
  // Install our signal handler.
  std::signal(SIGABRT, term::signal_handler);
  std::signal(SIGINT, term::signal_handler);
  std::signal(SIGTERM, term::signal_handler);

  term_raw_mode(true);
}

}  // namespace ultra::term
