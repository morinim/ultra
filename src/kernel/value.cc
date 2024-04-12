/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023, 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <iostream>

#include "kernel/value.h"
#include "kernel/nullary.h"
#include "kernel/gp/src/variable.h"
#include "utility/misc.h"

namespace ultra
{

///
/// \param[in] v value to be checked
/// \return      `true` if `v` isn't empty
///
bool has_value(const value_t &v) noexcept
{
  return !std::holds_alternative<std::monostate>(v);
}

///
/// \param[in] d domain to be checked
/// \return      `true` for numbers and strings
///
bool basic_data_type(domain_t d) noexcept
{
  static_assert(0 <= d_void && d_void < d_string);
  static_assert(0 <= d_int && d_int < d_string);
  static_assert(0 <= d_double && d_double < d_string);
  static_assert(0 <= d_string);

  return d <= d_string;
}

///
/// \param[in] v value to be checked
/// \return      `true` for numbers and strings
///
/// \related
/// value_t
///
bool basic_data_type(const value_t &v) noexcept
{
  return basic_data_type(static_cast<domain_t>(v.index()));
}

///
/// \param[in] v container object
/// \return      a pointer to the nullary object contained in `v` if present,
///              otherwise `nullptr`
///
const D_NULLARY *get_if_nullary(const value_t &v) noexcept
{
  return std::holds_alternative<const D_NULLARY *>(v) ?
         std::get<const D_NULLARY *>(v) : nullptr;
}

///
/// Streams a value_t object.
///
/// \param[out] o output stream
/// \param[in]  v value to be streamed
/// \return       a reference to the output stream
///
/// Mainly used for debugging. Both serialization and source code visualization
/// require different / more information.
///
std::ostream &operator<<(std::ostream &o, const value_t &v)
{
  switch (v.index())
  {
  case d_address: o << '[' << as_integer(std::get<D_ADDRESS>(v)) << ']'; break;
  case d_double:  o << std::get<D_DOUBLE>(v);                            break;
  case d_int:     o << std::get<D_INT>(v);                               break;
  case d_nullary: o << std::get<const D_NULLARY *>(v)->to_string();      break;
  case d_string:  o << std::quoted(std::get<D_STRING>(v));               break;
  case d_variable:o << std::get<const D_VARIABLE *>(v)->name();          break;
  case d_void:    o << "{}";                                             break;
  }

  return o;
}

}  // namespace ultra
