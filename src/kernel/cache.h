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

#if !defined(ULTRA_CACHE_H)
#define      ULTRA_CACHE_H

#include <mutex>
#include <optional>
#include <shared_mutex>

#include "kernel/fitness.h"
#include "kernel/hash_t.h"

namespace ultra
{

/// Type alias for the number of bits.
using bitwidth = unsigned;

///
/// \tparam LOCK_GROUP_SIZE set to `1` for maximum granularity (one lock per
///                         slot) or to the size of the table for a single
///                         mutex covering all slots. The default value
///                         balances performance and thread safety for most use
///                         cases.
///
/// This class implements a hash table that maps individuals' signatures to
/// their fitness.
/// It's primarily used by the `evaluator_proxy` class to optimise performance.
///
/// During evolution, semantically equivalent but syntactically distinct
/// individuals are often generated. By using this cache, the system avoids
/// redundant computations of shared information, resulting in significant
/// speed improvements.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE = 128>
class cache
{
public:
  cache() = default;  // empty cache
  explicit cache(bitwidth);

  void resize(bitwidth);
  [[nodiscard]] bitwidth bits() const;

  void clear();
  void clear(const hash_t &);

  void insert(const hash_t &, const F &);

  [[nodiscard]] std::optional<F> find(const hash_t &) const;

  [[nodiscard]] bool is_valid() const;

  // Serialization.
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  // Private support methods.
  [[nodiscard]] std::size_t index(const hash_t &) const noexcept;
  [[nodiscard]] std::size_t lock_index(std::size_t) const noexcept;

  // Private data members.
  struct slot
  {
    /// This is used as primary key for access to the table.
    hash_t hash;
    /// The stored fitness of an individual.
    F fitness;
    /// Valid slots are recognized comparing their seal with the current one.
    unsigned seal {0};
  };

  std::vector<slot> table_ {};
  mutable std::vector<std::shared_mutex> locks_ {};  // group-level locks

  std::uint64_t k_mask {0};
  decltype(slot::seal) seal_ {1};

  // Check the number of slots per lock.
  static_assert(LOCK_GROUP_SIZE > 0, "LOCK_GROUP_SIZE must be greater than 0");
};

#include "kernel/cache.tcc"

}  // namespace ultra

#endif  // include guard
