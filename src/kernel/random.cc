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

namespace
{

using namespace ultra::random;

///
/// Generates a unique seed for pseudo-random number generators.
///
/// \param[in] unpredictable if `true`, the initial seed is initialised from
///            `std::random_device`; otherwise a deterministic seed is used
/// \return    a unique seed value suitable for seeding a PRNG
///
/// This function returns a monotonically increasing seed value shared across
/// across the entire process. It's used to initialise thread-local random
/// engines so that each thread receives a distinct and independent random
/// sequence.
///
/// By default, the initial seed value is deterministic, ensuring reproducible
/// runs. When `unpredictable` is set to true, the initial seed is derived from
/// `std::random_device`, introducing non-determinism at the process level.
///
/// The function is thread-safe and lock-free. Seed generation relies on an
/// atomic counter with relaxed memory ordering, which is sufficient because no
/// inter-thread ordering constraints are required.
///
/// \see
/// - https://stackoverflow.com/a/77510422/3235496
/// - https://www.johndcook.com/blog/2016/01/29/random-number-generator-seed-mistakes/
///
[[nodiscard]] engine_t::result_type next_seed(bool unpredictable = false)
{
  static std::atomic<engine_t::result_type> process_seed(
    unpredictable ? std::random_device{}() : 1);

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
/// Every thread has its own generator initialized with a different seed.
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
/// `std::random_device`, ensuring that subsequently created random engines
/// are seeded unpredictably.
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
  [[maybe_unused]] auto _(next_seed(true));
}

}  // namespace ultra::random
