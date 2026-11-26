/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/hash_t.h"
#include "kernel/random.h"
#include "kernel/gp/individual.h"

#include "utility/timer.h"

#include "test/fixture1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

template<class FN>
std::uint64_t run_bench(std::string_view name, std::size_t iters, FN &&fn)
{
  constexpr std::size_t runs(5);

  // Warmup to stabilize caches, branch predictors and CPU frequency.
  const std::size_t warmup_iters(std::max<std::size_t>(1, iters / 20));
  volatile std::uint64_t warmup_sink(0);
  std::uint64_t warmup_acc(0);
  for (std::size_t i(0); i < warmup_iters; ++i)
  {
    const auto h(fn(i));
    warmup_acc ^= h.data[0] ^ h.data[1];
  }
  warmup_sink = warmup_acc;
  (void)warmup_sink;

  std::array<long long, runs> samples {};
  std::uint64_t acc(0);

  for (std::size_t r(0); r < runs; ++r)
  {
    std::uint64_t run_acc(0);

    ultra::timer t;
    for (std::size_t i(0); i < iters; ++i)
    {
      const auto h(fn(i));
      run_acc ^= h.data[0] ^ h.data[1];
    }
    samples[r] = t.elapsed().count();

    acc = run_acc;
  }

  std::ranges::sort(samples);
  const auto median(samples[runs / 2]);

  std::cout << std::setfill(' ') << std::setw(32) << std::left << name
            << " - Median: " << std::setw(5) << std::right << median
            << "ms  (min: " << std::setw(4) << samples.front()
            << "ms, max: " << std::setw(4) << samples.back()
            << "ms, checksum: 0x" << std::hex << acc << std::dec << ")\n";

  return acc;
}

int main()
{
  using namespace ultra;

  constexpr std::size_t bulk_size(4096);
  std::array<std::byte, bulk_size> buffer {};
  for (std::size_t i(0); i < bulk_size; ++i)
    buffer[i] = static_cast<std::byte>(i * 131u + 17u);

  // Pre-generated values for small-write `hash_sink` benchmark (mimics
  // `gp::individual::pack` streaming opcodes, ints and doubles).
  struct pack_item
  {
    std::uint16_t opcode;
    std::uint16_t arg_opcode;
    std::int32_t ival;
    double dval;
  };

  constexpr std::size_t n_items(16);
  std::array<pack_item, n_items> items {};
  for (std::size_t i(0); i < n_items; ++i)
    items[i] = {static_cast<std::uint16_t>(i + 1),
                static_cast<std::uint16_t>(i * 3 + 7),
                static_cast<std::int32_t>(i * 1009 - 500),
                static_cast<double>(i) * 1.25 + 0.5};

  // Volatile sink to prevent the compiler from eliding the benchmark loops.
  volatile std::uint64_t out(0);

  constexpr std::size_t bulk_iters(2'400'000);

  // -------------------------------------------------------------------------
  // 1. One-shot `hash128` on short variable-length inputs (1..63 bytes).
  {
    constexpr std::size_t iters(100'000'000);
    out ^= run_bench("hash128 (short 1..63 B)", iters,
                     [&](std::size_t i)
                     {
                       const auto len((i % 63) + 1);
                       const auto off((i * 7) % (bulk_size - len));
                       return hash::hash128(buffer.data() + off, len);
                     });
  }

  // -------------------------------------------------------------------------
  // 2. One-shot `hash128` on a 4 KiB buffer.
  std::uint64_t bulk_acc(0);
  {
    bulk_acc = run_bench("hash128 (bulk 4 KiB)", bulk_iters,
                         [&](std::size_t i)
                         {
                           return hash::hash128(buffer.data(), buffer.size(),
                                                static_cast<std::uint32_t>(i));
                         });
    out ^= bulk_acc;
  }

  // -------------------------------------------------------------------------
  // 3. `hash_sink` with small typed writes (2/4/8 B via `bytes_view`).
  std::uint64_t sink_small_acc(0);
  {
    constexpr std::size_t iters(4'000'000);
    sink_small_acc = run_bench(
      "hash_sink (small 2/4/8 B)", iters,
      [&](std::size_t i)
      {
        hash_sink sink(static_cast<std::uint32_t>(i));
        for (const auto &it : items)
        {
          sink.write(bytes_view(it.opcode));
          sink.write(bytes_view(it.arg_opcode));
          sink.write(bytes_view(it.ival));
          sink.write(bytes_view(it.dval));
        }
        return sink.final();
      });
    out ^= sink_small_acc;
  }

  // -------------------------------------------------------------------------
  // 4. `hash_sink` on a 4 KiB buffer fed in 64 B chunks.
  std::uint64_t sink_bulk_acc(0);
  {
    constexpr std::size_t chunk(64);
    const std::span<const std::byte> full(buffer);

    sink_bulk_acc = run_bench(
      "hash_sink (bulk 4 KiB / 64 B)", bulk_iters,
      [&](std::size_t i)
      {
        hash_sink sink(static_cast<std::uint32_t>(i));
        for (std::size_t off(0); off < full.size(); off += chunk)
          sink.write(full.subspan(off, chunk));
        return sink.final();
      });
    out ^= sink_bulk_acc;
  }

  // -------------------------------------------------------------------------
  // 5. End-to-end `gp::individual` mutation (exercises `pack`).
  {
    fixture1 f;
    f.prob.params.slp.code_length = 128;

    constexpr std::size_t pool_size(256);
    std::vector<gp::individual> pool;
    pool.reserve(pool_size);
    for (std::size_t i(0); i < pool_size; ++i)
      pool.emplace_back(f.prob);

    constexpr std::size_t iters(8'000'000);
    out ^= run_bench("gp::individual::mutation", iters,
                     [&](std::size_t i)
                     {
                       auto &ind(pool[i % pool_size]);
                       ind.mutation(f.prob, 1.0);
                       return ind.signature();
                     });
  }

  const bool ok(bulk_acc == sink_bulk_acc);
  if (!ok)
    std::cout << "PROBLEM: checksum mismatch between hash128 and hash_sink.\n";

  return !ok || out == 0;
}
