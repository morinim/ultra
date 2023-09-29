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

#include <sstream>

#include "kernel/terminal.h"

namespace ultra
{
///
/// \return a string representing the symbol
///
std::string terminal::display(format) const
{
  Expects(!std::holds_alternative<D_VOID>(data_));

  std::stringstream ss;

  if (nullary())
    ss << name() << "()";
  else
    ss << data_;

  return ss.str();
}

///
/// \return the value associated with the terminal (may be discarded / useless
///         in case of D_NULLARY terminal).
const value_t &terminal::value() const
{
  if (nullary())
    std::get<D_NULLARY>(data_)();

  return data_;
}

bool terminal::operator==(const terminal &rhs) const
{
  if (category() != rhs.category())
    return false;

  if (opcode() != rhs.opcode())
    return false;

  return data_ == rhs.data_;
}

bool operator!=(const terminal &lhs, const terminal &rhs)
{
  return !(lhs == rhs);
}

terminal terminal::random() const
{
  return terminal("EMPTY", D_VOID(), symbol::undefined_category);
}

}  // namespace ultra
