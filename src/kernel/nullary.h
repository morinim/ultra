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
/// A lambda function without arguments.
///
/// Function without arguments can be meaningful and not necessarily constant
/// (due to side effects). Such functions may have some hidden input, such as
/// global variables or the whole state of the system (time, free memory...).
///
/// A lambda function (aka anonymous function) is a literal for the function
/// type.
///
class nullary : public terminal
{
public:
  using terminal::terminal;

  [[nodiscard]] virtual value_t eval() const = 0;
  [[nodiscard]] value_t instance() const final { return this; }

  [[nodiscard]] bool is_arithmetic() const override { return false; }

  [[nodiscard]] std::string to_string(format = c_format) const
  { return name() + "()"; }
};

}  // namespace ultra

#endif  // include guard
