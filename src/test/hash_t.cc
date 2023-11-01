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
TEST_CASE("Murmur Hash")
{
  using std::byte;
  using ultra::hash_t;

  const unsigned hashbytes(128 / 8);

  byte *const key(new byte[256]);
  byte *const hashes(new byte[hashbytes * 256]);
  byte *const final(new byte[hashbytes]);

  std::memset(key, 0, 256);
  std::memset(hashes, 0, hashbytes * 256);
  std::memset(final, 0, hashbytes);

  // Hash keys of the form {0}, {0,1}, {0,1,2}... up to N=255, using 256-N as
  // the seed.
  for (unsigned i(0); i < 256; ++i)
  {
    key[i] = static_cast<byte>(i);

    auto h(ultra::hash::hash128(key, i, 256 - i));
    reinterpret_cast<std::uint64_t *>(&hashes[i * hashbytes])[0] = h.data[0];
    reinterpret_cast<std::uint64_t *>(&hashes[i * hashbytes])[1] = h.data[1];
  }

  // Then hash the result array.
  auto h(ultra::hash::hash128(hashes, hashbytes * 256, 0));
  reinterpret_cast<std::uint64_t *>(final)[0] = h.data[0];
  reinterpret_cast<std::uint64_t *>(final)[1] = h.data[1];

  // The first four bytes of that hash, interpreted as a little-endian integer,
  // is our verification value.
  const auto verification((static_cast<std::uint32_t>(final[0]) <<  0u) |
                          (static_cast<std::uint32_t>(final[1]) <<  8u) |
                          (static_cast<std::uint32_t>(final[2]) << 16u) |
                          (static_cast<std::uint32_t>(final[3]) << 24u));

  delete [] key;
  delete [] hashes;
  delete [] final;

  //----------

  CHECK(verification == 0x6384BA69);
}  // TEST_CASE("Murmur Hash")

}  // TEST_SUITE("HASH_T")
