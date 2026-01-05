/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_DE_INDIVIDUAL_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_DE_INDIVIDUAL_TCC)
#define      ULTRA_DE_INDIVIDUAL_TCC

///
/// Applies a mutating operation to a contiguous range of parameters.
///
/// \tparam F a callable with signature `void(value_type &)`
///
/// \param first first index (inclusive)
/// \param last  one past the last index (exclusive)
/// \param f     mutating function applied to each parameter in the range
///
/// \pre `first <= last <= parameters()`
/// \pre  No concurrent access.
///
/// \post The individual's signature is recomputed.
///
template<class F>
requires std::invocable<F &, individual::value_type &>
void individual::apply(std::size_t first, std::size_t last, F &&f)
{
  Expects(first <= last);
  Expects(last <= parameters());

  for (std::size_t i(first); i < last; ++i)
    std::invoke(f, genome_[i]);

  signature_ = hash();
}

template<class F>
requires std::invocable<F &, individual::value_type &>
void individual::apply(F &&f)
{
  apply(0, size(), std::forward<F>(f));
}

#endif  // include guard
