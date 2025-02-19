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
#include "kernel/symbol_set.h"
#include "kernel/gp/src/variable.h"
#include "utility/misc.h"

namespace ultra
{

///
/// Checks whether a givene `value_t` contains a value.
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
/// \related value_t
///
bool basic_data_type(const value_t &v) noexcept
{
  return basic_data_type(static_cast<domain_t>(v.index()));
}

///
/// \param[in] d domain to be checked
/// \return      `true` for numbers
///
bool numerical_data_type(domain_t d) noexcept
{
  return d == d_int || d == d_double;
}

///
/// \param[in] v value to be checked
/// \return      `true` for numbers
///
/// \related value_t
///
bool numerical_data_type(const value_t &v) noexcept
{
  return numerical_data_type(static_cast<domain_t>(v.index()));
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
  case d_ivector:
    o << '{';
    if (const auto &iv(std::get<D_IVECTOR>(v)); !iv.empty())
    {
      o << iv[0];
      for (std::size_t i(1); i < iv.size(); ++i)
        o << ' ' << iv[i];
    }
    o << '}';
  }

  return o;
}

///
/// \param[in]  in input stream
/// \param[in]  ss symbol set used for decoding `d_address` / `d_nullary`
/// \param[out] v  value_t coming from the input stream
/// \return       `true` if the object has been loaded correctly
///
/// \remark
/// `d_void` values are completely skipped.
///
/// \note
/// If the load operation isn't successful `v` isn't modified.
///
bool load(std::istream &in, const symbol_set &ss, value_t &v)
{
  int d;
  if (!(in >> d))
    return false;

  value_t t;
  switch (d)
  {
  case d_address:
    if (int x; in >> x)
      t = param_address(x);
    break;

  case d_double:
    if (double x; load_float_from_stream(in, &x))
      t = x;
    break;

  case d_int:
    if (int x; in >> x)
      t = x;
    break;

  case d_ivector:
    if (std::size_t size; in >> size)
    {
      D_IVECTOR iv(size);
      for (auto &e : iv)
        if (!(in >> e))
          break;
      t = iv;
    }
    break;

  case d_nullary:
    if (symbol::opcode_t x; in >> x)
      if (const auto *n = get_if<nullary>(ss.decode(x)))
        t = n;
    break;

  case d_string:
    if (std::string s; in >> s)
      t = s;
    break;

  case d_variable:
    if (std::string name; in >> name)
      if (const auto *n = get_if<D_VARIABLE>(ss.decode(name)))
        t = n;
    break;

  case d_void:
    break;
  }

  if (!has_value(t))
    return false;

  v = t;
  return true;
}

///
/// \param[out] out output stream
/// \param[in]  v   value_t to be saved
/// \return         `true` if `v` has been saved correctly
///
/// \remark
/// `d_void` values are completely skipped.
///
bool save(std::ostream &out, const value_t &v)
{
  out << v.index();

  if (v.index() != d_void)
    out << ' ';

  switch (v.index())
  {
  case d_address:
    out << as_integer(std::get<D_ADDRESS>(v));
    break;
  case d_double:
    save_float_to_stream(out, std::get<D_DOUBLE>(v));
    break;
  case d_int:
    out << std::get<D_INT>(v);
    break;
  case d_ivector:
  {
    const auto &iv(std::get<D_IVECTOR>(v));
    out << iv.size();

    for (std::size_t i(0); i < iv.size(); ++i)
      out << ' ' << iv[i];
    break;
  }
  case d_nullary:
    out << std::get<const D_NULLARY *>(v)->opcode();
    break;
  case d_string:
    out << std::get<D_STRING>(v);
    break;
  case d_variable:
    out << std::get<const D_VARIABLE *>(v)->name();
    break;
  case d_void:
    break;
  }

  return true;
}

}  // namespace ultra
