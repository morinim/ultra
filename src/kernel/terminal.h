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

  [[nodiscard]] virtual value_t instance() const = 0;
};

template<class T> concept Terminal = std::derived_from<T, terminal>;

}  // namespace ultra

#endif  // include guard
