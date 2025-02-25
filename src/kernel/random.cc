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

#include <atomic>

#include "kernel/random.h"

namespace
{

using namespace ultra::random;

// See:
// - https://stackoverflow.com/a/77510422/3235496
// - https://www.johndcook.com/blog/2016/01/29/random-number-generator-seed-mistakes/
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

  return (min_val + random::sup(2 * radius)) % n;
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
/// Sets the shared engine to an unpredictable state.
///
void randomize()
{
  [[maybe_unused]] auto _(next_seed(true));
}

}  // namespace ultra::random
