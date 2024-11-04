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

#if !defined(ULTRA_UTILITY_H)
#define      ULTRA_UTILITY_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

#include "kernel/value.h"

namespace ultra
{

// *******************************************************************
// Concepts
// *******************************************************************
template<class A> concept ArithmeticType = requires(A x, A y)
{
  {x + y} -> std::convertible_to<A>;
  {x - y} -> std::convertible_to<A>;
  {x * y} -> std::convertible_to<A>;
  {x / y} -> std::convertible_to<A>;
  {-x} -> std::convertible_to<A>;
  {x / double()} -> std::convertible_to<A>;
};

template<class A> concept OrderedArithmeticType =
  ArithmeticType<A> && std::totally_ordered<A>;

template<class A> concept ArithmeticFloatingType =
  OrderedArithmeticType<A>
  && (std::is_floating_point_v<A>
      || std::is_floating_point_v<typename A::value_type>);

// *******************************************************************
// Classes
// *******************************************************************
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

template <class T>
requires std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>
class revert_on_scope_exit
{
public:
  explicit revert_on_scope_exit(T &src) : val_ref_(src), orig_(src) {}

  ~revert_on_scope_exit() { val_ref_ = orig_; }

private:
  T &val_ref_;
  const T orig_;
};

namespace internal
{

template<class> struct closure_info;

template<class F>  // overloaded operator() (e.g. std::function)
struct closure_info
  : closure_info<decltype(&std::remove_reference_t<F>::operator())>
{
};

template<class R, class Arg>  // free functions
struct closure_info<R(Arg)>
{
  using arg_type = Arg;
  using return_type = R;
};

template<class R, class Arg>  // function pointers
struct closure_info<R(*)(Arg)> : closure_info<R(Arg)>
{
};

template<class R, class C, class Arg>  // member functions
struct closure_info<R(C::*)(Arg)> : closure_info<R(Arg)>
{
};

template<class R, class C, class Arg>  // const member functions (and lambdas)
struct closure_info<R(C::*)(Arg) const> : closure_info<R(C::*)(Arg)>
{
};

}  // namespace internal

///
/// Extracts the parameter type of a single-parameter callable object.
///
/// This is mainly used to extract the type of individual from a evaluator
/// function.
///
template<class F> using closure_arg_t =
  std::remove_cvref_t<typename internal::closure_info<std::remove_cvref_t<F>>::arg_type>;

///
/// Extracts the return type of a single-parameter callable object.
///
/// This is mainly used to extract the type of fitness from a evaluator
/// function.
///
template<class F> using closure_return_t =
  std::remove_cvref_t<typename internal::closure_info<std::remove_cvref_t<F>>::return_type>;

///
/// A very basic range type.
///
template<std::input_iterator Iterator>
class basic_range
{
public:
  basic_range(Iterator beg_it, Iterator end_it) noexcept
    : b_(beg_it), e_(end_it) {}

  [[nodiscard]] auto begin() const noexcept { return b_; }
  [[nodiscard]] auto end() const noexcept { return e_; }

  [[nodiscard]] auto rbegin() const noexcept
  { return std::reverse_iterator(e_); }

  [[nodiscard]] auto rend() const noexcept
  { return std::reverse_iterator(b_); }

private:
  Iterator b_;
  Iterator e_;
};

///
/// A utility class for dealing with the "problem" of noncopyable objects.
///
/// Typical use cases are classes containing a mutex or an unique id.
///
/// E.g. in order to copy objects containing a mutex you have to write a custom
/// copy constructor and copy assignment operator (breaking the *Rule of Zero*).
/// Often you don't need to copy the mutex to copy the object because the mutex
/// isn't part of the object's value, it's just there as a tool to protect
/// access.
///
template<class M>
class ignore_copy : public M
{
public:
  ignore_copy() = default;
  ignore_copy(const ignore_copy &) noexcept {}

  const ignore_copy &operator=(const ignore_copy &) const noexcept
  { return *this; }
};

///
/// An application-level numerical unique id.
///
class app_level_uid
{
public:
  app_level_uid() noexcept = default;

  [[nodiscard]] operator unsigned() const noexcept;

private:
  [[nodiscard]] static unsigned next_id() noexcept;

  const unsigned val_ {next_id()};
};

// *******************************************************************
// Functions
// *******************************************************************
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
template<std::floating_point T> [[nodiscard]] bool issmall(T v)
{
  static constexpr auto e(std::numeric_limits<T>::epsilon());

  return std::abs(v) < 2.0 * e;
}

///
/// \param[in] v value to check
/// \return      `true` if `v` is nonnegative
///
template<class T>
requires std::is_arithmetic_v<T>
[[nodiscard]] bool isnonnegative(T v)
{
  return v >= static_cast<T>(0);
}

///
/// \param[in] val a value to be rounded
/// \return        `val` rounded to a fixed, ultra-specific, number of decimals
///
template<ArithmeticFloatingType T>
[[nodiscard]] T round_to(T val, T float_epsilon = 0.0001)
{
  val /= float_epsilon;
  val = std::round(val);
  val *= float_epsilon;

  return val;
}

///
/// Reduced version of `boost::lexical_cast`.
///
/// \tparam T type we want to cast to
///
/// \return the content of the input string converted in an object of type `T`
///
template<class T> [[nodiscard]] T lexical_cast(const std::string &);
template<std::integral T> [[nodiscard]] T lexical_cast(const std::string &s)
{ return std::stoi(s); }
template<> [[nodiscard]] inline double lexical_cast(const std::string &s)
{ return std::stod(s); }
template<> [[nodiscard]] inline std::string lexical_cast(const std::string &s)
{ return s; }
template<class T> [[nodiscard]] T lexical_cast(const value_t &);
template<class T> [[nodiscard]] T lexical_cast(std::chrono::milliseconds);

///
/// Checks if an iterator is within a range.
///
/// \param[in] it    iterator to be checked
/// \param[in] range a given range
/// \return          `true` if `it` is within `range`
///
template<std::ranges::range R>
[[nodiscard]] bool iterator_of(std::ranges::iterator_t<const R> it,
                               const R &range)
{
  return std::ranges::any_of(range,
                             [it](const auto &v)
                             {
                               return std::addressof(v) == std::addressof(*it);
                             });
}

///
/// Find the index of a value contained in a continuous container.
///
/// \param[in] val       value to be found
/// \param[in] container continuous container
/// \return              the index of `val` in `container`
///
template<std::ranges::contiguous_range C>
[[nodiscard]] std::size_t get_index(const std::ranges::range_value_t<C> &val,
                                    const C &container)
{
  const auto *data(std::data(container));

  // `std::less` and `std::greater_equal` are required otherwise performing the
  // comparison with an object that isn't part of the container would be UB.
  Expects(std::greater_equal{}(&val, data));
  Expects(std::less{}(&val, data + std::size(container)));

  return std::addressof(val) - data;
}

///
/// \param[in] v1 a floating point number
/// \param[in] v2 a floating point number
/// \param[in] e  max relative error. If we want 99.999% accuracy then we
///               should pass `0.00001`
/// \return       `true` if the difference between `v1` and `v2` is *small*
///               compared to their magnitude
///
/// \note
/// Code from Bruce Dawson (modified considering Pavel Celba's comment):
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
///
template<std::floating_point T>
[[nodiscard]] bool almost_equal(T v1, T v2, T e = 0.00001)
{
  // Handles special values of `v1` / `v2 (infinity, Nan...).
  // `std::equal_to` (usually) avoids warnings with floating point comparison.
  if (std::equal_to()(v1, v2))
    return true;

  const T diff(std::abs(v1 - v2));

  // Check if the numbers are really close -- needed when comparing numbers
  // near zero.
  if (issmall(diff))
    return true;

  v1 = std::abs(v1);
  v2 = std::abs(v2);

  // Handles the `v1 == +inf` / `v2 == -inf` case.
  if (std::equal_to()(v1, v2) && std::equal_to()(diff, v1))
    return false;

  // In order to get consistent results, we always compare the difference to
  // the largest of the two numbers.
  const T largest(std::max(v1, v2));

  return diff <= largest * e;
}

template<std::integral T>
[[nodiscard]] bool almost_equal(T v1, T v2)
{
  return v1 == v2;
}

///
/// Serialises a floating point value.
///
/// \param[out] out the output stream
/// \param[in]  i   the floating-point value to be saved
/// \return         a reference to the output stream
///
/// Serialisation uses hexadecimal format for precision and performance:
/// - no rounding occurs in writing or reading a value formatted in this way;
/// - reading and writing such values can be faster with a well tuned I/O
///   library;
/// - fewer digits are required to represent values exactly.
///
template<std::floating_point T>
std::ostream &save_float_to_stream(std::ostream &out, T i)
{
  /// The `std::hexfloat` format is intended to dump out the exact
  /// representation of a floating point value so no truncation is performed in
  /// any way based on any stream setting.
  out << std::hexfloat << i;
  return out;
}

///
/// Deserialises a floating point value.
///
/// \param[in]  in the input stream
/// \param[out] i  the floating-point value to be loaded
/// \return        `true` if the operation is successful
///
template<std::floating_point T>
bool load_float_from_stream(std::istream &in, T *i)
{
  // Do NOT use `in >> std::hexfloat >> *i` for deserialisation (see
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81122).

  std::string str;
  if (!(in >> str))
    return false;

  str = trim(str);

  char *end;
  const double val(std::strtod(str.c_str(), &end));

  if (end == str.c_str() || *end != '\0')  // if no conversion can be
    return false;                          // performed, `end` is set
                                           // to `str.c_str()`

  *i = val;
  return true;
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
