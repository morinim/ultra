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

TEST_SUITE("HASH_T")
{

TEST_CASE("Type hash_t")
{
  const ultra::hash_t empty;
  CHECK(empty.empty());

  ultra::hash_t h(123, 345);
  CHECK(!h.empty());

  CHECK(h != empty);

  h.clear();
  CHECK(h.empty());

  CHECK(h == empty);
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

  ultra::hash_sink sink(1234);
  sink.write(std::span<const byte>(data.data(), 100));
  sink.write(std::span<const byte>(data.data() + 100, 156));

  const auto incremental(sink.final());

  CHECK(one_shot.data[0] == incremental.data[0]);
  CHECK(one_shot.data[1] == incremental.data[1]);
}

}  // TEST_SUITE("HASH_T")
