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
#include "kernel/gp/individual.h"

#include "utility/timer.h"

#include "test/fixture1.h"


int main()
{
  using namespace ultra;

  fixture1 f;
  constexpr std::size_t sup(10000);
  constexpr unsigned cycles(100000);

  // Automatically scales to system capabilities while ensuring at least one
  // thread for reads and writes.
  const auto n_threads(
    std::max<unsigned>(std::thread::hardware_concurrency(), 2));
  const auto r_threads(n_threads / 2);
  const auto w_threads(n_threads - r_threads);

  std::vector<std::uint64_t> reads(r_threads, 0), writes(w_threads, 0);

  cache<double> cache(16);

  // Variable length random creation.
  std::cout << "Generating " << sup << " signatures.\n";

  std::vector<std::pair<hash_t, double>> ind_db;
  ind_db.reserve(sup);

  for (std::size_t i(0); i < sup; ++i)
  {
    f.prob.params.slp.code_length = random::between(10, 200);

    const auto sig(gp::individual(f.prob).signature());
    const auto fit(static_cast<double>(i));
    ind_db.emplace_back(sig, fit);

    // Cache warm-up.
    cache.insert(sig, fit);

    if (i % (sup/10) == 0)
      std::cout << "Generated " << i << " signatures.\r" << std::flush;
  }
  std::cout << std::string(70, ' ') << "\r";

  const auto writer([&](unsigned idx)
  {
    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
      {
        cache.insert(ind_db[i].first, ind_db[i].second);
        ++writes[idx];
      }
  });

  const auto reader([&](unsigned idx)
  {
    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
        if (cache.find(ind_db[i].first))
          ++reads[idx];
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
}
