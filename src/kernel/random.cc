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
  Expects(radius);
  Expects(n > 1);

  if (radius >= n / 2)
    return random::sup(n);

  const auto min_val(base + n - radius);

  return (min_val + random::sup(2 * radius + 1)) % n;
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
/// This function initialises the shared seed generator using entropy from
/// `std::random_device`, so that subsequently created random engines are
/// usually seeded unpredictably.
///
/// Each thread owns its own thread-local random engine; therefore, only
/// engines created *after* this call are affected. Existing engines are
/// left unchanged.
///
/// This design allows deterministic and non-deterministic random behaviour
/// to coexist within the same program, depending on when engines are
/// initialised.
///
void randomize()
{
  using seed_t = engine_t::result_type;

  // std::random_device returns some value with external entropy
  // (platform-dependent). This entropy will become the new "starting point"
  // for future seeds.
  const auto entropy = static_cast<seed_t>(std::random_device{}());

  seed_t cur = process_seed.load(std::memory_order_relaxed);

  const auto inc = [](seed_t x) noexcept -> seed_t
  {
    using U = std::make_unsigned_t<seed_t>;

    // Increment performed in uintmax_t to avoid signed overflow UB.
    const auto x1 = static_cast<std::uintmax_t>(static_cast<U>(x)) + 1;
    return static_cast<seed_t>(static_cast<U>(x1));  // wraps by construction
  };

  // This is a standard "CAS loop" (compare-and-swap loop). We'll try to
  // change `process_seed` to a new value. If another thread changes it first,
  // we retry.
  for (;;)
  {
    // We want the next seed handed out to be based on entropy, so desired
    // starts as entropy.
    seed_t desired = entropy;

    // Best-effort: avoid rewinding if possible. If the type space has wrapped
    // (or we are near exhaustion), reuse is acceptable by design.
    if (desired <= cur)
      desired = inc(cur);

    // If the `process_seed` currently equals `cur`, replace it with `desired`
    // and return `true`.
    // Otherwise, updates `cur` to the actual current value of `process_seed`
    // and return `false`.
    if (process_seed.compare_exchange_weak(cur, desired,
                                           std::memory_order_relaxed,
                                           std::memory_order_relaxed))
      break;

    // On failure, `cur` is updated; retry with the new `cur`.
  }
}

}  // namespace ultra::random
