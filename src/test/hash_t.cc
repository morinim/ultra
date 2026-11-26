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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "kernel/hash_t.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

TEST_SUITE("HASH_T")
{

TEST_CASE("Type hash_t")
{
  constexpr ultra::hash_t empty;
  static_assert(empty.empty());
  CHECK(empty.empty());

  ultra::hash_t h(123, 345);
  CHECK(!h.empty());

  CHECK(h != empty);

  h.clear();
  CHECK(h.empty());

  CHECK(h == empty);

  SUBCASE("Ordering and constexpr operations")
  {
    constexpr ultra::hash_t a(1, 2);
    constexpr ultra::hash_t b(1, 3);
    constexpr ultra::hash_t c(2, 0);

    static_assert(a < b);
    static_assert(b < c);
    static_assert(a <= a);
    static_assert(c > b);

    CHECK(a < b);
    CHECK(b < c);
    CHECK(c > a);

    constexpr auto combined([]()
    {
      ultra::hash_t acc;
      acc.combine(ultra::hash_t(10, 20));
      acc.combine(ultra::hash_t(30, 40));
      return acc;
    }());
    static_assert(!combined.empty());
    CHECK(!combined.empty());

    ultra::hash_t reversed;
    reversed.combine(ultra::hash_t(30, 40));
    reversed.combine(ultra::hash_t(10, 20));
    CHECK(combined != reversed);
  }
}

// This should hopefully be a thorough and unambiguous test of whether the hash
// is correctly implemented.
TEST_CASE("murmurhash3")
{
  using std::byte;

  constexpr std::size_t hash_bytes(128 / 8);

  std::array<byte, 256> key {};
  std::array<byte, hash_bytes * 256> hashes {};
  std::array<byte, hash_bytes> final {};

  // Hash keys of the form {0}, {0,1}, {0,1,2}... up to N=255, using 256-N as
  // the seed.
  for (unsigned i(0); i < 256; ++i)
  {
    key[i] = static_cast<byte>(i);

    const auto h(ultra::hash::hash128(key.data(), i, 256 - i));

    std::memcpy(hashes.data() + i*hash_bytes, h.data, hash_bytes);
  }

  // Hash the result array.
  const auto h(ultra::hash::hash128(hashes.data(), hashes.size(), 0));

  std::memcpy(final.data(), h.data, hash_bytes);

  // First four bytes interpreted as a little-endian uint32.
  const auto verification((static_cast<std::uint32_t>(final[0]) <<  0u) |
                          (static_cast<std::uint32_t>(final[1]) <<  8u) |
                          (static_cast<std::uint32_t>(final[2]) << 16u) |
                          (static_cast<std::uint32_t>(final[3]) << 24u));

  //----------

  CHECK(verification == 0x6384BA69);
}

// Murmur hash incremental equivalence.
TEST_CASE("murmurhash3_sink")
{
  using std::byte;

  std::array<byte, 256> data{};

  for (std::size_t i(0); i < data.size(); ++i)
    data[i] = static_cast<byte>(i);

  const auto one_shot(ultra::hash::hash128(data.data(), data.size(), 1234));

  SUBCASE("Large dynamic chunks")
  {
    ultra::hash_sink sink(1234);
    sink.write(std::span<const byte>(data.data(), 100));
    sink.write(std::span<const byte>(data.data() + 100, 156));

    CHECK(sink.final() == one_shot);
  }

  SUBCASE("Empty writes")
  {
    // No writes at all.
    ultra::hash_sink empty_sink(1234);
    CHECK(empty_sink.final() == ultra::hash::hash128(nullptr, 0, 1234));

    // Empty spans with `data() == nullptr` and empty `std::string` before,
    // inside (with non-empty tail), and after non-empty writes.
    const std::string empty_str;

    ultra::hash_sink sink(1234);
    sink.write(std::span<const byte>{});
    sink.write(std::span<const byte, 0>{});
    sink.write(ultra::bytes_view(empty_str));
    sink.write(std::span<const byte>(data.data(), 5));
    sink.write(std::span<const byte>{});
    sink.write(std::span<const byte, 0>{});
    sink.write(ultra::bytes_view(empty_str));
    sink.write(std::span<const byte>(data.data() + 5, data.size() - 5));
    sink.write(std::span<const byte>{});

    CHECK(sink.final() == one_shot);
  }

  SUBCASE("Fixed-extent writes across block boundaries")
  {
    const std::uint16_t u16_a(0x1234);
    const std::uint16_t u16_b(0xabcd);
    const std::int32_t i32(0x13572468);
    const double d_a(3.141592653589793);
    const double d_b(-42.5);
    const std::string str("ultra-hash");

    // Sequence of sizes: 8 + 4 + 2 + 2 (=16 exact) + 8 + 4 + 8 (straddles 16)
    // + 2 + 10 = 46 bytes.
    std::vector<byte> raw;
    const auto append([&](std::span<const byte> bytes)
    {
      raw.insert(raw.end(), bytes.begin(), bytes.end());
    });

    append(ultra::bytes_view(d_a));
    append(ultra::bytes_view(i32));
    append(ultra::bytes_view(u16_a));
    append(ultra::bytes_view(u16_b));
    append(ultra::bytes_view(d_b));
    append(ultra::bytes_view(i32));
    append(ultra::bytes_view(d_a));
    append(ultra::bytes_view(u16_b));
    append(ultra::bytes_view(str));

    ultra::hash_sink sink(77);
    sink.write(ultra::bytes_view(d_a));
    sink.write(ultra::bytes_view(i32));
    sink.write(ultra::bytes_view(u16_a));
    sink.write(ultra::bytes_view(u16_b));
    sink.write(ultra::bytes_view(d_b));
    sink.write(ultra::bytes_view(i32));
    sink.write(ultra::bytes_view(d_a));
    sink.write(ultra::bytes_view(u16_b));
    sink.write(ultra::bytes_view(str));

    CHECK(sink.final() == ultra::hash::hash128(raw.data(), raw.size(), 77));
  }

  SUBCASE("Multi-block writes with non-empty tail")
  {
    for (std::size_t tail_len(1); tail_len < 16; ++tail_len)
    {
      ultra::hash_sink sink(1234);
      sink.write(std::span<const byte>(data.data(), tail_len));
      sink.write(std::span<const byte>(data.data() + tail_len,
                                       data.size() - tail_len));

      CHECK(sink.final() == one_shot);
    }
  }

  SUBCASE("Chunk-size sweep")
  {
    for (std::size_t chunk : {1, 2, 3, 4, 7, 8, 15, 16, 17, 32, 64})
    {
      ultra::hash_sink sink(1234);
      for (std::size_t off(0); off < data.size(); off += chunk)
      {
        const auto n(std::min(chunk, data.size() - off));
        sink.write(std::span<const byte>(data.data() + off, n));
      }

      CHECK(sink.final() == one_shot);
    }
  }
}

}  // TEST_SUITE("HASH_T")