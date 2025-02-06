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

#if !defined(ULTRA_RANDOM_H)
#define      ULTRA_RANDOM_H

#include <cstdlib>
#include <numeric>
#include <random>

#include "kernel/interval.h"

#include "utility/assert.h"
#include "utility/xoshiro256ss.h"

namespace ultra::random
{

enum class distribution {uniform, normal};

///
/// xoshiro256** (XOR/shift/rotate) is an all-purpose, rock-solid generator
/// (not a cryptographically secure generator). It has excellent (sub-ns)
/// speed, a state space (256 bits) that is large enough for any parallel
/// application, and it passes all tests we are aware of.
///
using engine_t = vigna::xoshiro256ss;

[[nodiscard]] engine_t &engine();
[[nodiscard]] std::size_t ring(std::size_t, std::size_t, std::size_t);
void randomize();

///
/// A specialization for floating point values of the `random::between(T, T)`
/// template function.
///
/// \param[in] min minimum random number
/// \param[in] sup upper bound
/// \return        a random `double` in the `[min;sup[` range
///
/// \see
/// For further details:
/// - https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3551.pdf
/// - https://stackoverflow.com/q/24566574/3235496
/// - https://stackoverflow.com/q/25222167/3235496
///
template<std::floating_point T>
[[nodiscard]] T between(T min, T sup)
{
  Expects(min < sup);

  std::uniform_real_distribution<T> d(min, sup);
  return d(engine());
}

///
/// Picks up a random integer value uniformly distributed in the set of
/// integers `{min, min+1, ..., sup-1}`.
///
/// \param[in] min minimum random number
/// \param[in] sup upper bound
/// \return        a random number in the `[min;sup[` range
///
/// \note
/// Contrary to boost usage this function does not take a closed range.
/// Instead it takes a half-open range (C++ usage and same behaviour of the
/// real number distribution).
///
template<std::integral T>
[[nodiscard]] T between(T min, T sup)
{
  Expects(min < sup);

  std::uniform_int_distribution<T> d(min, sup - 1);
  return d(engine());
}

template<class T>
requires std::is_enum_v<T>
[[nodiscard]] T between(T min, T sup)
{
  Expects(min < sup);

  return static_cast<T>(between<std::underlying_type_t<T>>(min, sup));
}

///
/// \param[in] sup upper bound
/// \return        a random number in the [0;sup[ range
///
/// \note This is a shortcut for: `between<T>(0, sup)`
///
template<class T>
requires std::is_arithmetic_v<T> || std::is_enum_v<T>
[[nodiscard]] T sup(T sup)
{
  return between(static_cast<T>(0), sup);
}

///
/// \param[in] c a STL container
/// \return      a random element of container `c`
///
template<std::ranges::sized_range C>
[[nodiscard]] const typename C::value_type &element(const C &c)
{
  Expects(c.size());

  return *std::next(
    c.begin(),
    static_cast<typename C::difference_type>(sup(c.size())));
}

///
/// \param[in] i a right-open interval
/// \return      a random element of `i`
///
template<ArithmeticScalar A>
[[nodiscard]] A element(const interval<A> &i)
{
  return between(i.min, i.sup);
}

///
/// \param[in] c a STL container
/// \return      a random element of container `c`
///
template<std::ranges::sized_range C>
[[nodiscard]] typename C::value_type &element(C &c)
{
  Expects(c.size());

  return *std::next(
    c.begin(),
    static_cast<typename C::difference_type>(sup(c.size())));
}

///
/// \param[in] p a probability (`[0;1]` range)
/// \return      `true` `p%` times
///
/// \note
/// `bool` values are produced according to the Bernoulli distribution.
///
[[nodiscard]] inline bool boolean(double p = 0.5)
{
  Expects(0.0 <= p);
  Expects(p <= 1.0);

  std::bernoulli_distribution d(p);
  return d(engine());
}

///
/// Used for ephemeral random constant generation.
///
/// \param[in] d  type of distribution
/// \param[in] p1 **minimum** for uniform distribution; **mean - stddev/2** for
///               normal distribution
/// \param[in] p2 **maximum** for uniform distribution, **mean + stddev/2** for
///               normal distribution
/// \return       a random number distributed according to distribution `d`
///
/// \note
/// For normal distribution:
/// - `p2 - p1` equals the standard deviation;
/// - `std::midpoint(p1, p2)` equals the mean.
///
template<class T>
requires std::is_arithmetic_v<T>
[[nodiscard]] T ephemeral(distribution d, T p1, T p2)
{
  Expects(p1 < p2);

  switch (d)
  {
  case distribution::normal:
    return std::normal_distribution<T>(std::midpoint(p1, p2),
                                       p2 - p1)(engine());
  default:
    return between(p1, p2);
  }
}

}  // namespace ultra::random

#endif  // include guard
