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

#if !defined(ULTRA_VALUE_H)
#define      ULTRA_VALUE_H

#include <iosfwd>
#include <string>
#include <variant>

namespace ultra
{

class nullary;

///
/// Numerical ID of supported data types.
///
enum domain_t {d_void = 0, d_int, d_double, d_nullary, d_string};

using D_VOID    = std::monostate;
using D_INT     =            int;
using D_DOUBLE  =         double;
using D_NULLARY =        nullary;
using D_STRING  =    std::string;

///
/// A variant containing the data types used by the interpreter for internal
/// calculations / output value and for storing examples.
///
using value_t = std::variant<D_VOID, D_INT, D_DOUBLE, D_NULLARY *, D_STRING>;

[[nodiscard]] bool operator==(const value_t &, const value_t &);
[[nodiscard]] bool has_value(const value_t &);

std::ostream &operator<<(std::ostream &, const value_t &);

}  // namespace ultra

#endif  // include guard
