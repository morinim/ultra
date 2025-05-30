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
/// Provides a surrogate for an evaluator to control access to it.
///
/// evaluator_proxy uses a custom internal hash table to cache fitness scores
/// of individuals.
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
  // Access to the real evaluator.
  E eva_;

  // Hash table cache.
  mutable cache<evaluator_fitness_t<E>> cache_;
};

#include "kernel/evaluator_proxy.tcc"

}  // namespace ultra

#endif  // include guard
