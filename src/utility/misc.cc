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

#include <atomic>
#include <charconv>
#include <sstream>
#include <system_error>
#include <thread>

namespace ultra
{

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if all elements in both strings are same (case
///                insensitively)
///
/// \warning Uses the C locale (`<cctype>`).
///
bool iequals(const std::string &lhs, const std::string &rhs)
{
  // KEEP `unsigned char`. Changing to `auto` or `char` could trigger UB in
  // case of negative values.
  return std::ranges::equal(lhs, rhs,
                            [](unsigned char c1, unsigned char c2)
                            { return std::tolower(c1) == std::tolower(c2); });
}

///
/// Checks whether a character sequence represents a valid integer.
///
/// \param[in] sv the input string view to test
/// \return       `true` if `sv` represents a valid integer, `false` otherwise
///
/// Determines whether the given string represents a valid base-10 integer,
/// allowing for an optional leading '+' or '-' sign. Leading and trailing
/// whitespace is ignored.
///
/// \note
/// Leading and trailing whitespace is ignored, but embedded whitespace
/// (e.g. "12 3") is not permitted.
///
bool is_integer(std::string_view sv)
{
  sv = trim(sv);

  if (sv.empty())
    return false;

  // Allow an optional leading sign.
  const std::size_t idx((sv[0] == '+' || sv[0] == '-') ? 1 : 0);
  if (idx == sv.size())  // string was only '+' or '-'
    return false;

  [[maybe_unused]] int _;
  const auto *first(sv.data() + idx);
  const auto *last(sv.data() + sv.size());
  const auto [ptr, ec] = std::from_chars(first, last, _);

  // `ec == errc()` means conversion succeeded.
  // `ptr == end()` ensures we consumed the entire string.
  return ec == std::errc() && ptr == last;
}

///
/// Checks whether a character sequence represents a valid finite number.
///
/// \param[in] sv the input string view to test
/// \return       `true` if `sv` represents a valid finite number, `false`
///               otherwise
///
/// Determines whether the given string represents a valid floating-point
/// number in base 10. Leading and trailing whitespace is ignored.
///
/// Parsing is performed using `std::from_chars`, making the check fast and
/// locale-independent. The entire trimmed input must be consumed by the
/// conversion for the function to return `true`.
///
/// By design, `std::from_chars` does not recognise a leading '+' sign outside
/// of the exponent. This function explicitly extends the accepted grammar to
/// allow an optional leading '+' for consistency with other numeric helpers.
///
/// \note
/// Leading and trailing whitespace is ignored, but embedded whitespace
/// (e.g. "1 2.3") is not permitted.
///
bool is_number(std::string_view sv)
{
  sv = trim(sv);
  if (sv.empty())
    return false;

  // Extend the accepted grammar: allow an optional leading '+':
  // "the plus sign is not recognized outside of the exponent" (see
  // https://en.cppreference.com/w/cpp/utility/from_chars.html)
  if (sv.front() == '+')
  {
    sv.remove_prefix(1);
    if (sv.empty())
      return false;
  }

  [[maybe_unused]] double value;
  const auto *first(sv.data());
  const auto *last(sv.data() + sv.size());

  const auto [ptr, ec] = std::from_chars(first, last, value);

  return ec == std::errc{} && ptr == last && std::isfinite(value);
}

///
/// Removes leading and trailing whitespace from a string view.
///
/// \param[in] sv the input string view to trim
/// \return       a trimmed view of `sv` with leading and trailing whitespace
///               removed
///
/// Returns a view of the input string with all leading and trailing whitespace
/// characters removed. Whitespace is defined according to `std::isspace` in
/// the current C locale.
/// This function does not allocate and does not modify the underlying
/// character sequence. The returned view refers to the same storage as the
/// input.
///
/// \note
/// The behaviour of this function depends on the global C locale used by
/// `std::isspace`.
///
/// \warning
/// The returned `std::string_view` is only valid as long as the underlying
/// character sequence referenced by `sv` remains alive.
///
std::string_view trim(std::string_view sv)
{
  // KEEP `unsigned char`. Changing to `auto` or `char` could trigger UB in
  // case of negative values.
  const auto is_space([](unsigned char c) { return std::isspace(c); });

  while (!sv.empty() && is_space(sv.front()))
    sv.remove_prefix(1);

  while (!sv.empty() && is_space(sv.back()))
    sv.remove_suffix(1);

  return sv;
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
    ss << ds.count() << ':';
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
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  }

  return ss.str();
}

app_level_uid::operator unsigned() const noexcept
{
  return val_;
}

unsigned app_level_uid::next_id() noexcept
{
  // The initialisation is thread-safe because it's static and C++11 guarantees
  // that static initialisation will be thread-safe. Subsequent access will be
  // thread-safe because it's an atomic.
  static std::atomic<unsigned> count(0);

  return count++;
}

}  // namespace ultra
