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

#if !defined(ULTRA_HASH_T_H)
#define      ULTRA_HASH_T_H

#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>

namespace ultra
{
///
/// A 128bit unsigned integer used as individual's signature / hash table
/// look-up key.
///
struct hash_t
{
  explicit hash_t(std::uint64_t a = 0, std::uint64_t b = 0) : data{a, b} {}

  /// Resets the content of the object.
  void clear() noexcept { data[0] = data[1] = 0; }

  /// Standard equality operator for hash signature.
  [[nodiscard]] bool operator==(const hash_t &) const noexcept = default;

  [[nodiscard]] bool operator<(const hash_t &rhs) const noexcept
  {
    return data[0] < rhs.data[0]
           || (data[0] == rhs.data[0] && data[1] < rhs.data[1]);
  }

  /// Used to combine multiple hashes.
  ///
  /// \note
  /// In spite of its handy bit-mixing properties, XOR is not a good way to
  /// combine hashes due to its commutativity (e.g. see
  /// https://stackoverflow.com/q/5889238/3235496).
  void combine(hash_t h) noexcept
  {
    // This is the simple algorithm used in `Apache.Commons.HashCodeBuilder`.
    // It uses simple prime number multiplication and is a special case of
    // Bob Jenkins' idea (`m * H(A) + H(B)`).
    data[0] = data[0] * 37 + h.data[0];
    data[1] = data[1] * 37 + h.data[1];

    // An alternative from Boost is:
    //data[0] ^= h.data[0] + 0x9e3779b9 + (data[0] << 6) + (data[0] >> 2);
    //data[1] ^= h.data[1] + 0x9e3779b9 + (data[1] << 6) + (data[1] >> 2);
  }

  /// We assume that a string of 128 zero bits means empty.
  [[nodiscard]] bool empty() const noexcept { return !data[0] && !data[1]; }

  // Serialization.
  bool load(std::istream &);
  bool save(std::ostream &) const;

  // Data members.
  std::uint_least64_t data[2];
};

std::ostream &operator<<(std::ostream &, hash_t);

///
/// MurmurHash3 (https://github.com/aappleby/smhasher) by Austin Appleby.
///
/// MurmurHash3 is a relatively simple non-cryptographic hash algorithm. It's
/// noted for being fast, with excellent distribution, avalanche behavior and
/// overall collision resistance.
///
/// \note
/// An interesting alternative is SpookyHash
/// (https://burtleburtle.net/bob/hash/spooky.html) by Bob Jenkins.
///
class murmurhash3
{
public:
  [[nodiscard]] static hash_t hash128(const void *const, std::size_t,
                                      std::uint32_t = 1973) noexcept;

private:
  [[nodiscard]] static std::uint64_t fmix(std::uint64_t) noexcept;
  [[nodiscard]] static std::uint32_t fmix(std::uint32_t) noexcept;
  template<std::integral T> [[nodiscard]] static T get_block(
    const T *, std::size_t) noexcept;
};

///
/// Hashes a single message in one call, return 128-bit output.
///
/// \param[in] data data stream to be hashed
/// \param[in] len  length, in bytes, of `data`
/// \param[in] seed initialization seed
/// \return         the signature of `data`
///
inline hash_t murmurhash3::hash128(const void *const data, std::size_t len,
                                   std::uint32_t seed) noexcept
{
  const auto n_blocks(len / 16);  // block size is 128bit

  const std::uint64_t c1(0x87c37b91114253d5), c2(0x4cf5ad432745937f);

  // Body.
  const auto *blocks(reinterpret_cast<const std::uint64_t *>(data));
  hash_t h(seed, seed);

  for (std::size_t i(0); i < n_blocks; ++i)
  {
    std::uint64_t k1(get_block(blocks, i * 2 + 0));
    std::uint64_t k2(get_block(blocks, i * 2 + 1));

    k1 *= c1;
    k1  = std::rotl(k1, 31);
    k1 *= c2;
    h.data[0] ^= k1;

    h.data[0] = std::rotl(h.data[0], 27);
    h.data[0] += h.data[1];
    h.data[0] = h.data[0] * 5 + 0x52dce729;

    k2 *= c2;
    k2  = std::rotl(k2, 33);
    k2 *= c1;
    h.data[1] ^= k2;

    h.data[1] = std::rotl(h.data[1], 31);
    h.data[1] += h.data[0];
    h.data[1] = h.data[1] * 5 + 0x38495ab5;
  }

  // Tail.
  auto tail(reinterpret_cast<const std::byte *>(data) + n_blocks * 16);

  std::uint64_t k1(0), k2(0);

  switch (len & 15)
  {
  case 15: k2 ^= std::uint64_t(tail[14]) << 48;  [[fallthrough]];
  case 14: k2 ^= std::uint64_t(tail[13]) << 40;  [[fallthrough]];
  case 13: k2 ^= std::uint64_t(tail[12]) << 32;  [[fallthrough]];
  case 12: k2 ^= std::uint64_t(tail[11]) << 24;  [[fallthrough]];
  case 11: k2 ^= std::uint64_t(tail[10]) << 16;  [[fallthrough]];
  case 10: k2 ^= std::uint64_t(tail[ 9]) << 8;   [[fallthrough]];
  case  9: k2 ^= std::uint64_t(tail[ 8]) << 0;
           k2 *= c2; k2  = std::rotl(k2, 33); k2 *= c1; h.data[1] ^= k2;
           [[fallthrough]];
  case  8: k1 ^= std::uint64_t(tail[ 7]) << 56;  [[fallthrough]];
  case  7: k1 ^= std::uint64_t(tail[ 6]) << 48;  [[fallthrough]];
  case  6: k1 ^= std::uint64_t(tail[ 5]) << 40;  [[fallthrough]];
  case  5: k1 ^= std::uint64_t(tail[ 4]) << 32;  [[fallthrough]];
  case  4: k1 ^= std::uint64_t(tail[ 3]) << 24;  [[fallthrough]];
  case  3: k1 ^= std::uint64_t(tail[ 2]) << 16;  [[fallthrough]];
  case  2: k1 ^= std::uint64_t(tail[ 1]) << 8;   [[fallthrough]];
  case  1: k1 ^= std::uint64_t(tail[ 0]) << 0;
           k1 *= c1; k1  = std::rotl(k1, 31); k1 *= c2; h.data[0] ^= k1;
  }

  // Finalization.
  h.data[0] ^= len;
  h.data[1] ^= len;

  h.data[0] += h.data[1];
  h.data[1] += h.data[0];

  h.data[0] = fmix(h.data[0]);
  h.data[1] = fmix(h.data[1]);

  h.data[0] += h.data[1];
  h.data[1] += h.data[0];

  return h;
}

inline std::uint64_t murmurhash3::fmix(std::uint64_t k) noexcept
{
  // The constants were generated by a simple simulated-annealing algorithm.
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccd;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53;
  k ^= k >> 33;

  return k;
}

inline std::uint32_t murmurhash3::fmix(std::uint32_t k) noexcept
{
  // The constants were generated by a simple simulated-annealing algorithm.
  k ^= k >> 16;
  k *= 0x85ebca6b;
  k ^= k >> 13;
  k *= 0xc2b2ae35;
  k ^= k >> 16;

  return k;
}

template<std::integral T>
inline T murmurhash3::get_block(const T *p, std::size_t i) noexcept
{
  // The reason we do a memcpy() instead of simply returning `p[i]` is because
  // doing it this way avoids violations of the strict aliasing rule.

  T tmp;
  std::memcpy(&tmp, p + i, sizeof(T));
  return tmp;
}

using hash = murmurhash3;

///
/// \param[in] t a value belonging to a fundamental types
/// \return      the value content viewed as a raw sequence of bytes
///
template <class T>
requires (std::integral<T> || std::floating_point<T>)
[[nodiscard]] std::span<const std::byte, sizeof(T)> bytes_view(const T &t)
{
  return std::span<const std::byte, sizeof(T)>{
    reinterpret_cast<const std::byte *>(std::addressof(t)), sizeof(T)};
}

[[nodiscard]] std::span<const std::byte> bytes_view(const std::string &);

}  // namespace ultra

#endif  // include guard
