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

#if !defined(ULTRA_EVALUATION_CONTEXT_H)
#define      ULTRA_EVALUATION_CONTEXT_H

#include <concepts>

namespace ultra
{

///
/// Describes whether an operation changed the semantic context used to
/// evaluate individuals.
///
/// When the context changes, cached evaluator results may no longer be valid.
///
enum class [[nodiscard]] evaluation_context
{
  unchanged,
  changed
};

///
/// Evaluator-like object whose cached state can be explicitly cleared.
///
template<class E>
concept ClearableEvaluator =
  requires(E &eva)
  {
    { eva.clear() } -> std::same_as<void>;
  };

///
/// Invalidates cached evaluator state if the evaluator exposes a compatible
/// `clear()` member function.
///
template<class E>
void invalidate_cache_if_supported(E &eva)
{
  if constexpr (ClearableEvaluator<E>)
    eva.clear();
}

}  // namespace ultra

#endif  // include guard
