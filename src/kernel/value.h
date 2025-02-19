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

#if !defined(ULTRA_VALUE_H)
#define      ULTRA_VALUE_H

#include <iosfwd>
#include <string>
#include <variant>
#include <vector>

namespace ultra
{

class nullary;
namespace src { class variable; }

/// Absolute address of a instruction / function.
enum class param_address : std::size_t;

constexpr param_address operator""_addr(unsigned long long a)
{
  return static_cast<param_address>(a);
}

///
/// Numerical ID of supported data types.
///
enum domain_t {d_void = 0, d_int, d_double, d_string, d_nullary, d_address,
               d_variable, d_ivector};

using D_VOID     =   std::monostate;
using D_INT      =              int;
using D_DOUBLE   =           double;
using D_STRING   =      std::string;
using D_NULLARY  =          nullary;
using D_ADDRESS  =    param_address;
using D_VARIABLE =    src::variable;
using D_IVECTOR  = std::vector<int>;

///
/// A variant containing the data types used by the interpreter for internal
/// calculations / output value and for storing examples.
///
using value_t = std::variant<D_VOID, D_INT, D_DOUBLE, D_STRING,
                             const D_NULLARY *, D_ADDRESS, const D_VARIABLE *,
                             D_IVECTOR>;

[[nodiscard]] bool basic_data_type(domain_t) noexcept;
[[nodiscard]] bool basic_data_type(const value_t &) noexcept;
[[nodiscard]] const D_NULLARY *get_if_nullary(const value_t &) noexcept;
[[nodiscard]] bool has_value(const value_t &) noexcept;
[[nodiscard]] bool numerical_data_type(domain_t) noexcept;
[[nodiscard]] bool numerical_data_type(const value_t &) noexcept;

std::ostream &operator<<(std::ostream &, const value_t &);

// Serialization.
class symbol_set;
bool load(std::istream &, const symbol_set &, value_t &);
bool save(std::ostream &, const value_t &);

}  // namespace ultra

#endif  // include guard
