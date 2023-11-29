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

#if !defined(ULTRA_CACHE_H)
#define      ULTRA_CACHE_H

#include <mutex>
#include <shared_mutex>

#include "kernel/environment.h"
#include "kernel/fitness.h"
#include "kernel/hash_t.h"

namespace ultra
{
///
/// Implements a hash table that links individuals' signature to fitness
/// (mainly used by the evaluator_proxy class).
///
/// During the evolution semantically equivalent (but syntactically distinct)
/// individuals are often generated and cache can give a significant speed
/// improvement avoiding the recalculation of shared information.
///
template<Fitness F>
class cache
{
public:
  explicit cache(unsigned);

  void clear();
  void clear(const hash_t &);

  void insert(const hash_t &, const F &);

  [[nodiscard]] const F *find(const hash_t &) const;

  [[nodiscard]] bool is_valid() const;

  // Serialization.
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  // Private support methods.
  [[nodiscard]] std::size_t index(const hash_t &) const;

  // Private data members.
  struct slot
  {
    /// This is used as primary key for access to the table.
    hash_t   hash;
    /// The stored fitness of an individual.
    F     fitness;
    /// Valid slots are recognized comparing their seal with the current one.
    unsigned seal;
  };

  mutable std::shared_mutex mutex_;

  const std::uint64_t k_mask;
  std::vector<slot>   table_;
  decltype(slot::seal) seal_;
};

#include "kernel/cache.tcc"

}  // namespace ultra

#endif  // include guard
