/**
 *  \file
 *
 *  \copyright Copyright (C) 2018-2024 Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see http://xoroshiro.di.unimi.it/
 */

#include "xoshiro256ss.h"

#include <algorithm>
#include <iostream>

namespace vigna
{

namespace
{

///
/// A fixed-increment version of Java 8's `SplittableRandom` generator.
///
/// The output of a `splitmix64` generator, seeded with a 64-bit seed, is used
/// to seed the xoroshiro / xoroshift1024* PRNGs (i.e. fill their initial
/// state).
///
/// \see https://dx.doi.org/10.1145/2714064.2660195
///
class splitmix64
{
public:
  explicit splitmix64(std::uint64_t seed) noexcept : x(seed) {}

  std::uint64_t next() noexcept
  {
    auto z(x += 0x9E3779B97F4A7C15);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    return z ^ (z >> 31);
  }

private:
  std::uint64_t x;  // the state can be seeded with any value
};  // class splitmix64

template<class T>
void seed_with_sm64(std::uint64_t seed, T &state)
{
  splitmix64 sm(seed);

  std::generate(state.begin(), state.end(), [&sm]{ return sm.next(); });
}

}  // namespace



constexpr xoshiro256ss::result_type xoshiro256ss::def_seed;
constexpr xoroshiro128p::result_type xoroshiro128p::def_seed;

///
/// Seeds the engine so that the initial state is determined by an integer.
///
/// \param[in] s a seed
///
/// The state must be seeded so that it is not everywhere zero. Having a 64-bit
/// seed, we use the `splitmix64` generator output to fill `state`.
///
void xoshiro256ss::seed(xoshiro256ss::result_type s) noexcept
{
  if (s == 0)
    s = def_seed;

  seed_with_sm64(s, state);
}

///
/// Seeds the engine with a specific state.
///
/// \param[in] s an initial state
///
/// \remark
/// The all-zero state is invalid for this generator. If `s` is all zero, the
/// engine is seeded using `def_seed` instead.
///
void xoshiro256ss::seed(const std::array<std::uint64_t, 4> &s) noexcept
{
  if (s == std::array<std::uint64_t, 4>{0, 0, 0, 0})
    seed(def_seed);
  else
    state = s;
}

///
/// Advances the internal state of the engine.
///
/// \param[in] z magnitude in the change of the internal state
///
/// By any means equivalent to `z` consecutive calls of `operator()`.
///
void xoshiro256ss::discard(std::uint64_t z) noexcept
{
  while (z--)
    operator()();
}

///
/// Writes to the output stream the representation of the current state.
///
/// \param[out] o output stream
/// \param[in]  e the engine
/// \return       the modified output stream
///
/// In the output, adjacent numbers are separated by one space character. The
/// representation depends on the stream's formatting flags; use `std::dec`
/// for a portable decimal representation.
///
std::ostream &operator<<(std::ostream &o, const xoshiro256ss &e)
{
  return o << e.state[0] << ' ' << e.state[1] << ' '
           << e.state[2] << ' ' << e.state[3];
}

///
/// Reads from the input stream a textual representation of the current state.
///
/// \param[in]     i input stream
/// \param[in,out] e the engine
/// \return          the modified input stream
///
/// The input is parsed according to the stream's formatting flags; use
/// `std::dec` when reading a decimal representation.
///
std::istream &operator>>(std::istream &i, xoshiro256ss &e)
{
  decltype(e.state) temp;
  if (i >> temp[0] >> temp[1] >> temp[2] >> temp[3])
    e.seed(temp);

  return i;
}

///
/// Seeds the engine so that the initial state is determined by an integer.
///
/// \param[in] s a seed
///
/// The state must be seeded so that it is not everywhere zero. Having a 64-bit
/// seed, we use the `splitmix64` generator output to fill `state`.
///
void xoroshiro128p::seed(xoroshiro128p::result_type s) noexcept
{
  if (s == 0)
    s = def_seed;

  seed_with_sm64(s, state);
}

///
/// Seeds the engine with a specific state.
///
/// \param[in] s an initial state
///
/// \remark
/// The all-zero state is invalid for this generator. If `s` is all zero, the
/// engine is seeded using `def_seed` instead.
///
void xoroshiro128p::seed(const std::array<std::uint64_t, 2> &s) noexcept
{
  if (s == std::array<std::uint64_t, 2>{0, 0})
    seed(def_seed);
  else
    state = s;
}

///
/// Advances the internal state of the engine.
///
/// \param[in] z magnitude in the change of the internal state
///
/// By any means equivalent to `z` consecutive calls of `operator()`.
///
void xoroshiro128p::discard(std::uint64_t z) noexcept
{
  while (z--)
    operator()();
}

///
/// Writes to the output stream the representation of the current state.
///
/// \param[out] o output stream
/// \param[in]  e the engine
/// \return       the modified output stream
///
/// In the output, adjacent numbers are separated by one space character. The
/// representation depends on the stream's formatting flags; use `std::dec`
/// for a portable decimal representation.
///
std::ostream &operator<<(std::ostream &o, const xoroshiro128p &e)
{
  return o << e.state[0] << ' ' << e.state[1];
}

///
/// Reads from the input stream a textual representation of the current state.
///
/// \param[in]     i input stream
/// \param[in,out] e the engine
/// \return          the modified input stream
///
/// The input is parsed according to the stream's formatting flags; use
/// `std::dec` when reading a decimal representation.
///
std::istream &operator>>(std::istream &i, xoroshiro128p &e)
{
  decltype(e.state) temp;
  if (i >> temp[0] >> temp[1])
    e.seed(temp);

  return i;
}

}  // namespace vigna
