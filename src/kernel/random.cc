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

#include "kernel/random.h"

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace
{

/// Single global atomic counter shared by `next_seed()` and `randomize()`.
std::atomic<ultra::random::engine_t::result_type> process_seed {1};

///
/// Generates a unique seed for pseudo-random number generators.
///
/// \return a unique seed value suitable for seeding a PRNG
///
/// This function returns a process-wide sequence of seed values. It is used to
/// initialise thread-local random engines so that each thread typically
/// receives a distinct seed.
///
/// By default, the initial seed value is deterministic, ensuring reproducible
/// runs. Calling `ultra::random::randomize` introduces non-determinism at the
/// process level.
///
/// The function is thread-safe and lock-free. Seed generation relies on an
/// atomic counter with relaxed memory ordering, which is sufficient because no
/// inter-thread ordering constraints are required.
///
/// \see
/// - https://stackoverflow.com/a/77510422/3235496
/// - https://www.johndcook.com/blog/2016/01/29/random-number-generator-seed-mistakes/
///
[[nodiscard]] ultra::random::engine_t::result_type next_seed() noexcept
{
  return process_seed.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

namespace ultra::random
{

///
/// Returns a random number in a modular arithmetic system.
///
/// \param[in] base   a base number
/// \param[in] radius maximum distance from the `base` number
/// \param[in] n      modulus
/// \return           a random number in the
///                   `[base - radius, base + radius] mod n` interval
///
std::size_t ring(std::size_t base, std::size_t radius, std::size_t n)
{
  Expects(base < n);
  Expects(n > 1);

  if (radius == 0) [[unlikely]]
    return base;

  if (radius >= n / 2)
    return random::sup(n);

  const auto start(base + n - radius);

  return (start + random::sup(2 * radius + 1)) % n;
}

///
/// Every thread has its own generator initialised with a different seed.
///
engine_t &engine()
{
  thread_local engine_t prng(next_seed());

  return prng;
}

///
/// Switches the random subsystem to an unpredictable state.
///
/// This function "re-bases" the process-wide seed counter so that random
/// engines created _after_ this call are typically seeded unpredictably.
///
/// Implementation notes / assumptions:
/// - `engine_t::result_type` is assumed to be an _unsigned_ integer type
///   at least 32 bits wide. This guarantees that `cur + 1` is well-defined
///   (wrap-around semantics) and provides a large seed space;
/// - the entropy used to re-base the counter is drawn from a small bounded
///   interval, because `randomize()` is expected to be called only a few
///   times and we do not need a huge entropy pool here;
/// - seed generation is thread-safe and lock-free: a CAS loop updates the
///   atomic counter using relaxed memory ordering (no inter-thread ordering
///   constraints are required).
///
/// Each thread owns its own thread-local random engine; therefore, only
/// engines created _after_ this call are affected. Existing engines are
/// left unchanged.
///
/// This design allows deterministic and non-deterministic random behaviour
/// to coexist within the same program, depending on when engines are
/// initialised.
///
void randomize()
{
  using seed_t = engine_t::result_type;

  // Contract: we rely on wrap-around behaviour (unsigned) and on a reasonably
  // large seed space (>= 32 bits).
  static_assert(std::is_integral_v<engine_t::result_type>,
                "engine_t::result_type must be an integral type");
  static_assert(std::numeric_limits<engine_t::result_type>::digits >= 32,
                "engine_t::result_type must be at least 32 bits wide");
  static_assert(std::is_unsigned_v<seed_t>,
                "engine_t::result_type must be unsigned");

  // std::random_device provides platform-dependent external entropy.
  std::random_device rd;

  // LCG (Park–Miller). Very small engine state and fast.
  std::minstd_rand gen(rd());

  // Bounded entropy interval used to re-base the process seed counter.
  // Large enough to avoid trivial values, small enough to avoid edge cases.
  constexpr seed_t lo(1000), hi(10000000);
  std::uniform_int_distribution<seed_t> dist(lo, hi);

  const seed_t entropy(static_cast<seed_t>(dist(gen)));

  // Snapshot of the current counter. Note: in the CAS loop, `cur` may be
  // updated to the latest observed value on failure.
  seed_t cur(process_seed.load(std::memory_order_relaxed));

  // This is a standard "CAS loop" (compare-and-swap loop). We'll try to
  // change `process_seed` to a new value. If another thread changes it first,
  // we retry.
  for (;;)
  {
    // Prefer rebasing to entropy, but never move the counter backwards if we
    // can avoid it (helps preserve uniqueness for subsequently created
    // engines).
    seed_t desired = entropy;

    if (desired <= cur)
      desired = cur + 1;  // well-defined due to unsigned contract

    // If the `process_seed` currently equals `cur`, replace it with `desired`
    // and return `true`.
    // Otherwise, updates `cur` to the actual current value of `process_seed`
    // and return `false`.
    if (process_seed.compare_exchange_weak(cur, desired,
                                           std::memory_order_relaxed,
                                           std::memory_order_relaxed))
      break;

    // On failure, `cur` now holds the latest value; retry.
  }
}

}  // namespace ultra::random
