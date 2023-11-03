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

#if !defined(ULTRA_UTILITY_H)
#define      ULTRA_UTILITY_H

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "kernel/value.h"

namespace ultra
{
[[nodiscard]] bool is_number(std::string);
[[nodiscard]] bool iequals(const std::string &, const std::string &);
[[nodiscard]] std::string replace(std::string, const std::string &,
                                  const std::string &);
[[nodiscard]] std::string replace_all(std::string, const std::string &,
                                      const std::string &);
[[nodiscard]] std::string trim(const std::string &);

///
/// Check if a value is almost equal to zero.
///
/// \tparam T type we want to check
///
/// \param[in] v value to check
/// \return      `true` if `v` is less than `epsilon`-tolerance
///
/// \note
/// `epsilon` is the smallest `T`-value that can be added to `1.0` without
/// getting `1.0` back (this is a much larger value than `DBL_MIN`).
///
template<class T> [[nodiscard]] bool issmall(T v)
{
  static constexpr auto e(std::numeric_limits<T>::epsilon());

  return std::abs(v) < 2.0 * e;
}

///
/// \param[in] v value to check
/// \return      `true` if `v` is nonnegative
///
template<class T> [[nodiscard]] bool isnonnegative(T v)
{
  return v >= static_cast<T>(0);
}

///
/// Reduced version of `boost::lexical_cast`.
///
/// \tparam T type we want to cast to
///
/// \param[in] s a string
/// \return      the content of string `s` converted in an object of type `T`
///
template<class T> [[nodiscard]] T lexical_cast(const std::string &s)
{ return std::stoi(s); }
template<> [[nodiscard]] inline double lexical_cast(const std::string &s)
{ return std::stod(s); }
template<> [[nodiscard]] inline std::string lexical_cast(const std::string &s)
{ return s; }
template<class T> [[nodiscard]] T lexical_cast(const value_t &);

///
/// A RAII class to restore the state of a stream to its original state.
///
/// `iomanip` manipulators are "sticky" (except `setw` which only affects the
/// next insertion). Often we need a way to apply an arbitrary number of
/// manipulators to a stream and revert the state to whatever it was before.
///
/// \note
/// An alternative is to shuffle everything into a temporary `stringstream` and
/// finally put that on the real stream (which has never changed its flags at
/// all). This approach is exception-safe but a little less performant.
///
class ios_flag_saver
{
public:
  explicit ios_flag_saver(std::ios &s) : s_(s), flags_(s.flags()),
                                         precision_(s.precision()),
                                         width_(s.width())
  {}

  ios_flag_saver(const ios_flag_saver &) = delete;
  ios_flag_saver &operator=(const ios_flag_saver &) = delete;

  ~ios_flag_saver()
  {
    s_.flags(flags_);
    s_.precision(precision_);
    s_.width(width_);
  }

private:
  std::ios &s_;
  std::ios::fmtflags flags_;
  std::streamsize precision_;
  std::streamsize width_;
};
#define SAVE_FLAGS(s) ios_flag_saver save ## __LINE__(s)

///
/// \param[in] v1 a floating point number
/// \param[in] v2 a floating point number
/// \param[in] e  max relative error. If we want 99.999% accuracy then we
///               should pass `0.00001`
/// \return       `true` if the difference between `v1` and `v2` is *small*
///               compared to their magnitude
///
/// \note Code from Bruce Dawson:
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
///
template<class T>
[[nodiscard]]
bool almost_equal(T v1, T v2, T e = 0.00001)
{
  const T diff(std::abs(v1 - v2));

  // Check if the numbers are really close -- needed when comparing numbers
  // near zero.
  if (issmall(diff))
    return true;

  v1 = std::abs(v1);
  v2 = std::abs(v2);

  // In order to get consistent results, we always compare the difference to
  // the largest of the two numbers.
  const T largest(std::max(v1, v2));

  return diff <= largest * e;
}

///
/// \param[out] out the output stream
/// \param[in]  i   the floating-point value to be saved
/// \return         a reference to the output stream
///
template<std::floating_point T>
std::ostream &save_float_to_stream(std::ostream &out, T i)
{
  static_assert(std::is_floating_point<T>::value,
                "save_float_to_stream requires a floating point type");

  SAVE_FLAGS(out);

  out << std::fixed << std::scientific
      << std::setprecision(std::numeric_limits<T>::digits10 + 1)
      << i;

  return out;
}

///
/// \param[in]  in the input stream
/// \param[out] i  the floating-point value to be loaded
/// \return        `true` if the operation is successful
///
template<std::floating_point T>
bool load_float_from_stream(std::istream &in, T *i)
{
  static_assert(std::is_floating_point_v<T>,
                "load_float_from_stream requires a floating point type");

  SAVE_FLAGS(in);

  return !!(in >> std::fixed >> std::scientific
               >> std::setprecision(std::numeric_limits<T>::digits10 + 1)
               >> *i);
}

template<typename T> concept IsEnum = std::is_enum_v<T>;

///
/// Encapsulate the logic to convert a scoped enumeration element to its
/// integer value.
///
/// \tparam E a scoped enumeration
///
/// \param[in] v element of an enum class
/// \return      the integer value of `v`
///
template<IsEnum E>
[[nodiscard]] constexpr std::underlying_type_t<E> as_integer(E v)
{
  return static_cast<std::underlying_type_t<E>>(v);
}

///
/// A generic function to "print" any scoped enum.
///
/// \tparam E a scoped enumeration
///
/// \param[in, out] s an output stream
/// \param[in]      v element of an enum class
/// \return           the modified output stream
///
template<IsEnum E> std::ostream &operator<<(std::ostream &s, E v)
{
  return s << as_integer(v);
}

}  // namespace ultra

#endif  // include guard
