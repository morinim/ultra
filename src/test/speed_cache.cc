/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cstdlib>
#include <thread>

#include "kernel/cache.h"
#include "kernel/random.h"

#include "utility/timer.h"

template<class CACHE>
unsigned test(CACHE &cache,
              const std::vector<std::pair<ultra::hash_t, double>> &db)
{
  using namespace ultra;

  std::cout << "Cache warming up\n";
  for (std::size_t i(0); i < db.size(); ++i)
    cache.insert(hash_t(i, i), static_cast<double>(i));

  // Automatically scales to system capabilities while ensuring at least one
  // thread for reads and writes.
  const auto n_threads(
    std::max<unsigned>(std::thread::hardware_concurrency(), 2));
  const auto r_threads(n_threads / 2);
  const auto w_threads(n_threads - r_threads);

  std::vector<std::uint64_t> reads(r_threads, 0), writes(w_threads, 0);

  constexpr unsigned cycles(10000);

  const auto writer([&](unsigned thread_idx)
  {
    const auto sup(db.size());
    const auto start(random::sup(sup));

    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
      {
        const auto index((start + i) % sup);
        cache.insert(db[index].first, db[index].second);
        ++writes[thread_idx];
      }
  });

  const auto reader([&](unsigned thread_idx)
  {
    const auto sup(db.size());
    const auto start(random::sup(sup));

    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
        if (cache.find(db[(start + i) % sup].first))
          ++reads[thread_idx];
  });

  std::vector<std::thread> threads;

  std::cout << "Starting " << r_threads << " readers and " << w_threads
            << " writers.\n";
  for (unsigned i(0); i < std::max(r_threads, w_threads); ++i)
  {
    // Alternating between reader and writer thread creation could smooth
    // resource usage.
    if (i < r_threads)
      threads.emplace_back(reader, i);
    if (i < w_threads)
      threads.emplace_back(writer, i);
  }

  std::cout << "Threads started.\n"
            << "Performing read/write test.\n";

  ultra::timer t;

  for (auto &thread: threads)
    thread.join();

  std::cout << "Test finished.\n";

  const auto e_ms(t.elapsed().count());
  std::cout << "\nTime elapsed: " << e_ms << "ms\n";

  {
    std::uint64_t total_reads(0);
    std::cout << "\nREADS\n";
    for (unsigned i(0); i < reads.size(); ++i)
    {
      std::cout << "Thread " << i << ": " << reads[i] << '\n';
      total_reads += reads[i];
    }
    std::cout << "Total: " << total_reads << '\n';
    std::cout << "Reads/s: " << 1000 * total_reads / e_ms << '\n';
  }

  {
    std::uint64_t total_writes(0);
    std::cout << "\nWRITES\n";
    for (unsigned i(0); i < writes.size(); ++i)
    {
      std::cout << "Thread " << i << ": " << writes[i] << '\n';
      total_writes += writes[i];
    }
    std::cout << "Total: " << total_writes << '\n';
    std::cout << "Writes/s: " << 1000 * total_writes / e_ms << '\n';
  }

  return e_ms;
}

int main()
{
  using namespace ultra;

  constexpr std::size_t sup(10000);

  // Variable length random creation.
  std::cout << "Generating " << sup << " signatures.\n";

  std::vector<std::pair<hash_t, double>> ind_db;
  ind_db.reserve(sup);

  for (std::size_t i(0); i < sup; ++i)
  {
    const hash_t sig(i, std::numeric_limits<std::uint64_t>::max() - i);
    const auto fit(static_cast<double>(i));
    ind_db.emplace_back(sig, fit);

    if (i % (sup/10) == 0)
      std::cout << "Generated " << i << " signatures.\r" << std::flush;
  }
  std::cout << std::string(70, ' ') << "\r";

  unsigned default_ms;
  {
    std::cout << "\n\nDEFAULT GROUP SIZE\n";
    cache<double> cache_dflt(16);
    default_ms = test(cache_dflt, ind_db);
  }

  unsigned one_per_slot_ms;
  {
    std::cout << "\n\nONE MUTEX PER SLOT\n";
    cache<double, 1> cache(16);
    one_per_slot_ms = test(cache, ind_db);
  }

  unsigned one_for_all;
  {
    std::cout << "\n\nONE MUTEX PER TABLE\n";
    cache<double, std::size_t(1) << 16> cache(16);
    one_for_all = test(cache, ind_db);
  }

  std::cout << "\nSUMMARY\n"
            << "Default: " << default_ms << "ms"
            << "  One per slot: " << one_per_slot_ms << "ms"
            << "  One for all: " << one_for_all << "ms\n";
}
