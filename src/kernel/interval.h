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

#include <utility>

namespace ultra
{

template<class A> concept ArithmeticScalar = std::is_arithmetic_v<A>;

///
/// Right-open interval.
///
/// `interval_t{m, u}` specifies the half-open (left-closed, right-open)
/// interval `[m, u[`.
///
template<ArithmeticScalar T> using interval_t = std::pair<T, T>;

template<std::floating_point T1, std::floating_point T2>
constexpr auto interval(T1 m, T2 u)
{
  Expects(m < u);
  return interval_t<decltype(m + u)>(m, u);
}

template<std::integral T1, std::integral T2>
constexpr auto interval(T1 m, T2 u)
{
  Expects(std::cmp_less(m, u));
  return interval_t<decltype(m + u)>(m, u);
}

}  // namespace ultra

#endif
