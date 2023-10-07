/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_NULLARY_H)
#define      ULTRA_NULLARY_H

#include "kernel/terminal.h"

namespace ultra
{
///
/// A function without arguments.
///
/// Function without arguments can be meaningful and not necessarily constant
/// (due to side effects). Such functions may have some hidden input, such as
/// global variables or the whole state of the system (time, free memory...).
///
class nullary : public terminal
{
public:
  using terminal::terminal;

  [[nodiscard]] std::string to_string(
    const value_t & = {}, format = c_format) const override
  { return name() + "()"; }
};

}  // namespace ultra

#endif  // include guard
