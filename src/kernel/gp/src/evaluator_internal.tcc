/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SRC_EVALUATOR_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_SRC_EVALUATOR_INTERNAL_TCC)
#define      ULTRA_SRC_EVALUATOR_INTERNAL_TCC

namespace internal
{

/// Extracts the type of a single example from a dataset.
///
/// This helper metafunction determines the type of elements yielded by a
/// dataset, abstracting over the two supported dataset forms.
template<class D, bool = is_multi_dataset_v<D>>
struct dataset_example_impl;

template<class D>
struct dataset_example_impl<D, true>
{
  using type =
    std::remove_cvref_t<decltype(*std::declval<D>().selected().begin())>;
};

template<class D>
struct dataset_example_impl<D, false>
{
  using type =
    std::remove_cvref_t<decltype(*std::declval<D>().begin())>;
};

template<class D>
using dataset_example_t = typename dataset_example_impl<D>::type;

}  // namespace internal

#endif  // include guard
