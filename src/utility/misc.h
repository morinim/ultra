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

#if !defined(ULTRA_MISC_H)
#define      ULTRA_MISC_H

#include "kernel/value.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <ranges>

namespace ultra
{

using namespace std::chrono_literals;

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

/// Trait to enable bitmask operators for a given enum type.
///
/// \tparam E enum type to check
///
template<class E> struct is_bitmask_enum : std::false_type {};


/// Convenience variable template to check if an enum is enabled for bitmask
/// operations.
///
/// You will specialise this later for your enums.
///
template<class E>
inline constexpr bool is_bitmask_enum_v = is_bitmask_enum<E>::value;


///
/// Concept that checks if a type is an enum and has bitmask operations
/// enabled.
///
/// This ensures that bitwise operators are only available for enums that
/// explicitly opt in.
///
template<class E>
concept bitmask_enum = is_bitmask_enum_v<E> && std::is_enum_v<E>;

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

// Internal trait: extracts argument and return type from evaluator-style
// callables. Assumes exactly one non-templated unary `operator()`.
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

[[nodiscard]] bool is_integer(std::string_view);
[[nodiscard]] bool is_number(std::string_view);
[[nodiscard]] bool iequals(const std::string &, const std::string &);
[[nodiscard]] std::string replace(std::string, const std::string &,
                                  const std::string &);
[[nodiscard]] std::string replace_all(std::string, const std::string &,
                                      const std::string &);
[[nodiscard]] std::string_view trim(std::string_view);

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
/// Bitwise OR operator for bitmask-enabled enums.
///
/// \param[in] lhs left-hand operand
/// \param[in] rhs right-hand operand
/// \return        result of bitwise the operation
///
template<bitmask_enum E>
[[nodiscard]] constexpr E operator|(E lhs, E rhs) noexcept
{
  return static_cast<E>(as_integer(lhs) | as_integer(rhs));
}

///
/// Bitwise AND operator for bitmask-enabled enums.
///
/// \param[in] lhs left-hand operand
/// \param[in] rhs right-hand operand
/// \return        result of bitwise the operation
///
template<bitmask_enum E>
[[nodiscard]] constexpr E operator&(E lhs, E rhs) noexcept
{
  return static_cast<E>(as_integer(lhs) & as_integer(rhs));
}

///
/// Bitwise XOR operator for bitmask-enabled enums.
///
/// \param[in] lhs left-hand operand
/// \param[in] rhs right-hand operand
/// \return        result of bitwise the operation
///
template<bitmask_enum E>
[[nodiscard]] constexpr E operator^(E lhs, E rhs) noexcept
{
  return static_cast<E>(as_integer(lhs) ^ as_integer(rhs));
}

///
/// Bitwise NOT operator for bitmask-enabled enums.
///
/// \param[in] value the enum value to negate
/// \return          result of bitwise the operation
///
template<bitmask_enum E>
[[nodiscard]] constexpr E operator~(E value) noexcept
{
  return static_cast<E>(~as_integer(value));
}


///
/// Helper function to check if a specific flag is set in a bitmask.
///
/// \tparam E bitmask-enabled enum type
///
/// \param[in] value the combined bitmask value
/// \param[in] flag  the flag(s) to test for
/// \return          `true` if all bits in `flag` are set in `value`, `false`
///                  otherwise
/// This uses bitwise AND to check if all bits in `flag` are also set in
/// `value`.
///
/// \remark
/// Edge case: `has_flag(value, E{}) == true`; this is mathematically
/// consistent and is what most libs do.
///
template<bitmask_enum E>
[[nodiscard]] constexpr bool has_flag(E value, E flag) noexcept
{
  return (value & flag) == flag;
}

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
[[nodiscard]] bool isnonnegative(T v) noexcept
{
  return v >= static_cast<T>(0);
}

///
/// \param[in] val a value to be rounded
/// \return        `val` rounded to a fixed, ultra-specific, number of decimals
///
template<ArithmeticFloatingType T>
[[nodiscard]] T round_to(T val, T float_epsilon = T(0.0001))
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
/// Checks whether an iterator refers to an element within a given range.
///
/// \tparam R a range type
///
/// \param[in] it iterator to be tested
/// \param[in] r  range to test against
/// \return       `true` if `it` refers to an element of `r`, `false` otherwise
///
/// The check is performed with compile-time dispatch based on the iterator
/// category:
/// - **contiguous iterators** are checked in constant time using address
///   bounds;
/// - **random-access iterators** are checked in constant time using ordering;
/// - **other forward iterators** fall back to a linear scan.
///
/// The function does not dereference `it` in the fast paths and does not
/// assume any particular relationship between iterator value and container
/// ownership beyond what the iterator category guarantees.
///
/// \complexity
/// - *O(1)* for contiguous and random-access iterators;
/// - *O(n)* for other forward iterators.
///
template<std::ranges::range R>
[[nodiscard]] bool iterator_of(std::ranges::iterator_t<const R> it, const R &r)
{
  using It = std::ranges::iterator_t<const R>;

  if constexpr (std::contiguous_iterator<It>)
  {
    const auto *p(std::to_address(it));
    const auto *b(std::to_address(r.begin()));
    const auto *e(std::to_address(r.end()));
    return b <= p && p < e;
  }
  else if constexpr (std::random_access_iterator<It>)
  {
    // This avoids relying on `<=`, which some custom iterators implement
    // poorly.
    return !(it < r.begin()) && it < r.end();
  }
  else
  {
    for (auto cur(r.begin()); cur != r.end(); ++cur)
      if (cur == it)
        return true;

    return false;
  }
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
///               should pass `0.0001`
/// \return       `true` if the difference between `v1` and `v2` is *small*
///               compared to their magnitude
///
/// \note
/// Code from Bruce Dawson (modified considering Pavel Celba's comment):
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
///
template<std::floating_point T>
[[nodiscard]] bool almost_equal(T v1, T v2, T e = 0.0001)
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
[[nodiscard]] bool almost_equal(T v1, T v2) noexcept
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
  SAVE_FLAGS(out);

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
/// Supports both decimal and hexadecimal floating point expressions, as well
/// as infinity and Nan.
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

///
/// Calculates the Hamming distance between two ranges.
///
/// \tparam R range type
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparsion
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs`
///
/// Hamming distance between two containers of equal length is the number of
/// positions at which the corresponding symbols are different.
///
template<std::ranges::range R>
[[nodiscard]] unsigned hamming_distance(const R &lhs, const R &rhs)
{
  Expects(std::ranges::distance(lhs) == std::ranges::distance(rhs));
  return std::transform_reduce(lhs.begin(), lhs.end(), rhs.begin(), 0u,
                               std::plus{}, std::not_equal_to{});
}

}  // namespace ultra

#endif  // include guard
