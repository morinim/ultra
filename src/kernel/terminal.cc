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

#include <exception>
#include <sstream>

#include "kernel/terminal.h"

namespace ultra
{
bool is_terminal(const symbol *s)
{
  return dynamic_cast<const terminal *>(s);
}

bool is_terminal(const symbol &s)
{
  return is_terminal(&s);
}

bool is_arithmetic(const symbol *s)
{
  return dynamic_cast<const arithmetic_terminal *>(s);
}

bool is_arithmetic(const symbol &s)
{
  return is_arithmetic(&s);;
}

///
/// \param[in] v a value
/// \return      a string representing the terminal
///
std::string arithmetic_terminal::to_string(const value_t &v, format) const
{
  std::stringstream ss;
  ss << v;
  return ss.str();
}

}  // namespace ultra
