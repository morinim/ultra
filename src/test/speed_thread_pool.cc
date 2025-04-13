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

#include "utility/thread_pool.h"
#include "utility/timer.h"

#include <algorithm>
#include <iostream>

using namespace std::chrono_literals;

void task()
{
  std::this_thread::sleep_for(0.1s);
}

void async_based(unsigned THREADS, unsigned GENERATIONS)
{
  const auto task_completed(
    [](const auto &future)
    {
      return future.wait_for(0ms) == std::future_status::ready;
    });

  std::cout << "Starting async simulation...";
  ultra::timer t;
  for (unsigned n(GENERATIONS); n; --n)
  {
    std::vector<std::future<void>> tasks;

    for (unsigned i(0); i < THREADS; ++i)
      tasks.push_back(std::async(std::launch::async, task));

    while (!std::ranges::all_of(tasks, task_completed))
      std::this_thread::sleep_for(50ms);
  }

  const auto e_ms(t.elapsed().count());
  std::cout << " test finished. Time elapsed: " << e_ms << "ms\n";
}

void thread_pool_based(unsigned THREADS, unsigned GENERATIONS)
{
  ultra::thread_pool tp(THREADS);

  std::cout << "Starting thread pool simulation... ";
  ultra::timer t;
  for (unsigned n(GENERATIONS); n; --n)
  {
    for (unsigned i(0); i < THREADS; ++i)
      tp.execute(task);

    tp.wait();
  }

  const auto e_ms(t.elapsed().count());
  std::cout << " test finished. Time elapsed: " << e_ms << "ms\n";
}

int main()
{
  const unsigned GENERATIONS(100);

  std::cout << "SINGLE THREAD\n";
  {
    async_based(1, GENERATIONS);
    thread_pool_based(1, GENERATIONS);
  }

  const auto THREADS(
    std::max<unsigned>(2, std::thread::hardware_concurrency()));

  // Async-based execution.
  std::cout << "\n\nMULTIPLE THREADS (" << THREADS << ")\n";
  async_based(THREADS, GENERATIONS);
  thread_pool_based(THREADS, GENERATIONS);
}
