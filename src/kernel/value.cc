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

#include <iostream>

#include "kernel/value.h"
#include "kernel/nullary.h"
#include "utility/misc.h"

namespace ultra
{

///
/// \return `true` if values are equal
///
/// Notes:
/// - values of different type are considered different (`1 != 1.0`);
/// - nullary objects are considered equal if they point to the same address;
/// - real values are compared using an approximation;
/// - everything else is compared using the specific `==` operator.
///
bool operator==(const value_t &lhs, const value_t &rhs)
{
  if (lhs.index() != rhs.index())
    return false;

  switch (lhs.index())
  {
  case d_double:
    return almost_equal(std::get<D_DOUBLE>(lhs), std::get<D_DOUBLE>(rhs));
  case d_int:
    return std::get<D_INT>(lhs) == std::get<D_INT>(rhs);
  case d_nullary:
    return std::get<const D_NULLARY *>(lhs) == std::get<const D_NULLARY *>(rhs);
  case d_string:
    return std::get<D_STRING>(lhs) == std::get<D_STRING>(rhs);
  case d_void:
    return true;
  default:
    return false;
  }
}

///
/// \param[in] v value to be checked
/// \return      `true` if `v` isn't empty
///
bool has_value(const value_t &v)
{
  return !std::holds_alternative<std::monostate>(v);
}

///
/// \param[in] v container object
/// \return      a pointer to the nullary object contained in `v` if present,
///              otherwise `nullptr`
///
const D_NULLARY *get_if_nullary(const value_t &v)
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
std::ostream &operator<<(std::ostream &o, const value_t &v)
{
  switch (v.index())
  {
  case d_double:  o << std::get<D_DOUBLE>(v);                       break;
  case d_int:     o << std::get<D_INT>(v);                          break;
  case d_nullary: o << std::get<const D_NULLARY *>(v)->to_string(); break;
  case d_string:  o << std::get<D_STRING>(v);                       break;
  case d_void:    o << "{}";                                        break;
  }

  return o;
}

}  // namespace ultra
