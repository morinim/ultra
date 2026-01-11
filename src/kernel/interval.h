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

#if !defined(ULTRA_INTERVAL_H)
#define      ULTRA_INTERVAL_H

#include "utility/assert.h"

#include <numeric>
#include <utility>

namespace ultra
{

template<class A> concept ArithmeticScalar =
  std::is_arithmetic_v<A> && !std::same_as<A, bool>;


///
/// Right-open interval.
///
/// `interval{m, u}` represents the half-open (left-closed, right-open)
/// interval `[m, u[`.
///
template<ArithmeticScalar T>
struct interval
{
  constexpr interval(T m, T s) : min(m), sup(s)
  {
    Expects(m < s);
  }

  template<std::floating_point T1, std::floating_point T2>
  constexpr interval(T1 m, T2 s) : interval(static_cast<T>(m),
                                            static_cast<T>(s))
  {
    Expects(m < s);
  }

  template<std::integral T1, std::integral T2>
  constexpr interval(T1 m, T2 s) : interval(static_cast<T>(m),
                                            static_cast<T>(s))
  {
    Expects(std::cmp_less(m, s));
    Expects(std::in_range<T>(m));
    Expects(std::in_range<T>(s));
  }

  template<std::floating_point T1, std::floating_point T2>
  constexpr interval(const std::pair<T1, T2> &p)
    : interval(static_cast<T>(p.first), static_cast<T>(p.second))
  {
    Expects(p.first < p.second);
  }

  template<std::integral T1, std::integral T2>
  constexpr interval(const std::pair<T1, T2> &p)
    : interval(static_cast<T>(p.first), static_cast<T>(p.second))
  {
    Expects(std::cmp_less(p.first, p.second));
    Expects(std::in_range<T>(p.first));
    Expects(std::in_range<T>(p.second));
  }

  [[nodiscard]] bool is_valid() const noexcept
  {
    return min < sup;
  }

  T min;
  T sup;
};  // class interval

}  // namespace ultra

#endif
