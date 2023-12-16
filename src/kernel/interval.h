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

template<ArithmeticScalar T1, ArithmeticScalar T2>
constexpr std::pair<T1, T2> interval(T1 &&m, T2 &&u)
{
  return std::pair<T1, T2>(std::forward<T1>(m), std::forward<T2>(u));
}

}  // namespace ultra

#endif
