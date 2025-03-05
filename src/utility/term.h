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

#include <csignal>

#if !defined(ULTRA_CONSOLE_H)
#define      ULTRA_CONSOLE_H

namespace ultra
{

///
/// Used to continuously monitoring the keyboard.
///
class term
{
public:
  term();
  ~term();

  [[nodiscard]] bool user_stop() const;

private:
  static void reset();
  static void signal_handler(int);
};

/// This global object sets the terminal appropriately at program start. The
/// destructor restores the initial state.
///
/// `console.user_stop()` is used for checking stop condition.
extern term console;

}  // namespace ultra

#endif  // include guard
