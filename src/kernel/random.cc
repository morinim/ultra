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

#include "kernel/random.h"

namespace ultra::random
{

///
/// The shared random engine generator.
///
/// Every thread has its own generator.
/// The numbers produced will be the same every time the program is run.
///
thread_local engine_t engine;

///
/// Initalizes the random number generator.
///
/// \param[in] s the seed for the random number generator
///
/// The seed is used to initalize the random number generator. With the same
/// seed the numbers produced will be the same every time the program is run.
///
/// \note
/// A common method to seed a PRNG is using the current time (`std::time(0)`).
/// It works... but the preferred way in Ultra is the `randomize` method (which
/// is based on `std::random_device`).
///
void seed(unsigned s)
{
  engine.seed(s);
}

///
/// Sets the shared engine to an unpredictable state.
///
void randomize()
{
  std::random_device rd;
  seed(rd());
}

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

}  // namespace ultra::random
