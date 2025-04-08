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

int main()
{
  using namespace ultra;

  const auto THREADS(
    std::max<unsigned>(2, std::thread::hardware_concurrency()));

  const unsigned GENERATIONS(100);

  // Async-based execution.
  {
    const auto task_completed(
      [](const auto &future)
      {
        return future.wait_for(0ms) == std::future_status::ready;
      });

    std::cout << "Starting async-based simulation\n";
    ultra::timer t;
    for (unsigned n(GENERATIONS); n; --n)
    {
      std::vector<std::future<void>> tasks;

      for (unsigned i(0); i < THREADS; ++i)
        tasks.push_back(std::async(std::launch::async, task));

      while (!std::ranges::all_of(tasks, task_completed))
        std::this_thread::sleep_for(100ms);
    }

    const auto e_ms(t.elapsed().count());
    std::cout << "Test finished.\nTime elapsed: " << e_ms << "ms\n";
  }

  // Thread pool-based execution.
  {
    thread_pool tp(THREADS);

    std::cout << "Starting async-based simulation\n";
    ultra::timer t;
    for (unsigned n(GENERATIONS); n; --n)
    {
      for (unsigned i(0); i < THREADS; ++i)
        tp.execute(task);

      tp.wait();
    }

    const auto e_ms(t.elapsed().count());
    std::cout << "Test finished.\nTime elapsed: " << e_ms << "ms\n";
  }
}
