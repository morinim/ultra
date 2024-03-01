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

#include "utility/misc.h"
#include "kernel/nullary.h"

namespace ultra
{
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if all elements in both strings are same (case
///                insensitively)
///
bool iequals(const std::string &lhs, const std::string &rhs)
{
  return std::ranges::equal(lhs, rhs,
                            [](auto c1, auto c2)
                            { return std::tolower(c1) == std::tolower(c2); });
}

///
/// \param[in] s the string to be tested
/// \return      `true` if `s` contains a number
///
bool is_number(std::string s)
{
  s = trim(s);

  char *end;
  const double val(std::strtod(s.c_str(), &end));  // if no conversion can be
                                                   // performed, `end` is set
                                                   // to `s.c_str()`
  return end != s.c_str() && *end == '\0' && std::isfinite(val);
}

///
/// \param[in] s the input string
/// \return      a copy of `s` with spaces removed on both sides of the string
///
/// \see http://stackoverflow.com/a/24425221/3235496
///
std::string trim(const std::string &s)
{
  auto front = std::ranges::find_if_not(
    s, [](auto c) { return std::isspace(c); });

  // The search is limited in the reverse direction to the last non-space value
  // found in the search in the forward direction.
  auto back = std::find_if_not(s.rbegin(), std::make_reverse_iterator(front),
                               [](auto c) { return std::isspace(c); }).base();

  return {front, back};
}

///
/// Replaces the first occurrence of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace(std::string s,
                    const std::string &from, const std::string &to)
{
  const auto start_pos(s.find(from));
  if (start_pos != std::string::npos)
    s.replace(start_pos, from.length(), to);

  return s;
}

///
/// Replaces all occurrences of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace_all(std::string s,
                        const std::string &from, const std::string &to)
{
  if (!from.empty())
  {
    std::size_t start(0);
    while ((start = s.find(from, start)) != std::string::npos)
    {
      s.replace(start, from.length(), to);
      start += to.length();  // in case `to` contains `from`, like replacing
                             // "x" with "yx"
    }
  }

  return s;

  // With std::regex it'd be something like:
  //     s = std::regex_replace(s, std::regex(from), to);
  // (possibly escaping special characters in the `from` string)
}

///
/// Converts a `value_t` to `double`.
///
/// \param[in] v value that should be converted to `double`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns `0.0`
///
/// This function is useful for:
/// * debugging purpose;
/// * symbolic regression and classification task (the value returned by
///   the interpreter will be used in a "numeric way").
///
/// \note
/// This is not the same of `std::get<T>(v)`.
///
template<>
double lexical_cast<double>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::get<D_DOUBLE>(v);
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<double>(std::get<D_STRING>(v));
  default:        return 0.0;
  }
}

template<>
int lexical_cast<int>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return static_cast<int>(std::get<D_DOUBLE>(v));
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<int>(std::get<D_STRING>(v));
  default:        return 0;
  }
}

///
/// Converts a `value_t` to `std::string`.
///
/// \param[in] v value that should be converted to `std::string`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns an empty string
///
/// This function is useful for debugging purpose.
///
template<>
std::string lexical_cast<std::string>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::to_string(std::get<D_DOUBLE>(v));
  case d_int:     return std::to_string(   std::get<D_INT>(v));
  case d_string:  return std::get<D_STRING>(v);
  case d_nullary: return get_if_nullary(v)->name();
  default:        return {};
  }
}

template<>
std::string lexical_cast<std::string>(std::chrono::milliseconds d)
{
  using namespace std;
  using namespace std::chrono_literals;

  const auto ds(chrono::duration_cast<chrono::days>(d));
  const auto hrs(chrono::duration_cast<chrono::hours>(d - ds));
  const auto mins(chrono::duration_cast<chrono::minutes>(d - ds - hrs));
  const auto secs(chrono::duration_cast<chrono::seconds>(d - ds - hrs - mins));

  std::stringstream ss;

  if (ds.count())
    ss << std::setw(4) << ds.count() << ':';
  if (ds.count() || hrs.count())
    ss << std::setw(2) << std::setfill('0') << hrs.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2) << std::setfill('0') << mins.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2);

  ss << std::setfill('0') << secs.count();

  if (ds.count() == 0 && hrs.count() == 0 && mins.count() == 0)
  {
    const auto ms(chrono::duration_cast<chrono::milliseconds>(d - ds - hrs
                                                              - mins - secs));
    ss << '.' << ms.count();
  }

  return ss.str();
}

}  // namespace ultra
