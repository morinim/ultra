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

#if !defined(ULTRA_EVALUATOR_PROXY_H)
#define      ULTRA_EVALUATOR_PROXY_H

#include "kernel/cache.h"
#include "kernel/evaluator.h"

#include "utility/log.h"

namespace ultra
{
///
/// Provides controlled access to an evaluator with transparent caching.
///
/// \tparam E an `Evaluator` type
///
/// `evaluator_proxy` acts as a surrogate for an `Evaluator`, adding an
/// internal cache that stores fitness values keyed by individual signatures.
/// This avoids repeated evaluation of semantically equivalent individuals and
/// allows expensive evaluators to be reused efficiently.
///
/// The proxy preserves the semantics of the underlying evaluator while
/// extending its capabilities with:
/// - memoisation of fitness values;
/// - optional fast (approximate) evaluation;
/// - optional persistence of both evaluator state and cache contents.
///
/// The cache is logically mutable: cache updates do not alter the observable
/// behaviour of the evaluator. For this reason, most member functions are
/// `const`.
///
template<Evaluator E>
class evaluator_proxy
{
public:
  evaluator_proxy(E, bitwidth);

  // Serialization.
  bool load(std::istream &);
  bool save(std::ostream &) const;

  bool load_cache(std::istream &) const;
  bool save_cache(std::ostream &) const;

  void clear() const;
  void clear(const hash_t &) const;

  [[nodiscard]] evaluator_fitness_t<E> operator()(
    const evaluator_individual_t<E> &) const;

  [[nodiscard]] evaluator_fitness_t<E> fast(
    const evaluator_individual_t<E> &) const;

  [[nodiscard]] const E &core() const noexcept;

private:
  E eva_;  // wrapped evaluator

  // Cache storing fitness values indexed by individual signatures.
  mutable cache<evaluator_fitness_t<E>> cache_;
};

#include "kernel/evaluator_proxy.tcc"

}  // namespace ultra

#endif  // include guard
