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
#include "utility/misc.h"

namespace ultra
{

namespace
{

struct compare_val
{
  bool operator()(D_DOUBLE a, D_DOUBLE b) { return almost_equal(a, b); }
  bool operator()(D_NULLARY, D_NULLARY) { return true; }
  template<class T> bool operator()(T a, T b) { return a == b; }
  template<class A, class B> bool operator()(A, B) { return false; }
};

}

///
/// \return `true` if values are equal
///
/// Notes:
/// - values of different type are considered different (`1 != 1.0`);
/// - nullary objects are considered equal (real comparison is handled
///   elsewhere. See `ultra::function`);
/// - real values are compared using an approximation;
/// - everything else is compared using the specific `==` operator.
///
bool operator==(const value_t &lhs, const value_t &rhs)
{
  return std::visit(compare_val(), lhs, rhs);
}

bool operator!=(const value_t &lhs, const value_t &rhs)
{
  return !(lhs == rhs);
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
/// Streams a value_t object.
///
/// \param[out] o output stream
/// \param[in]  v value to be streamed
/// \return       a reference to the output stream
///
std::ostream &operator<<(std::ostream &o, const value_t &v)
{
  // A more general implementation at
  // https://stackoverflow.com/a/47169101/3235496

  std::visit([&](auto &&arg) { o << arg; }, v);
  return o;
}

}  // namespace ultra
