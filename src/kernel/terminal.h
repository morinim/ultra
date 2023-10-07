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

#if !defined(ULTRA_TERMINAL_H)
#define      ULTRA_TERMINAL_H

#include "kernel/symbol.h"

namespace ultra
{
///
/// A terminal might be a variable (input to the program), a numeric value or a
/// function taking no arguments (e.g. `move-north()`).
///
class terminal : public symbol
{
public:
  using symbol::symbol;

  virtual value_t eval() const = 0;

  [[nodiscard]] virtual std::string to_string(
    const value_t &, format = c_format) const override;
};

}  // namespace ultra

#endif  // include guard
