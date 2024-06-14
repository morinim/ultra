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

#include <thread>

#include "utility/ts_queue.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("ts_queue")
{

TEST_CASE("Size")
{
  using namespace ultra;

  ts_queue<int> queue;
  const unsigned n(10);

  {
    const auto producer([&queue]()
    {
      for (unsigned i(0); i < n; ++i)
        queue.push(i);
    });

    std::jthread producer_thread(producer);
  }

  CHECK(queue.size() == n);
  CHECK(!queue.empty());
}

TEST_CASE("try_pop")
{
  using namespace ultra;

  ts_queue<int> queue;
  const int n(10000);


  int sum(0);
  {
    const auto producer([&queue]()
    {
      for (int i(1); i <= n; ++i)
        queue.push(i);
    });

    const auto consumer([&queue, &sum]()
    {
      for (int j(0); j < n; ++j)
        if (auto val(queue.try_pop()); val)
          sum += *val;
        else
          --j;
    });

    std::jthread producer_thread(producer);

    // Give the producer thread a lead
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::jthread consumer_thread(consumer);
  }

  const int expected_sum(n * (n + 1) / 2);
  CHECK(sum == expected_sum);

  CHECK(queue.empty());
}

TEST_CASE("pop 1")
{
  using namespace ultra;

  ts_queue<int> queue;

  const int n(50);
  const std::chrono::milliseconds delay{1};
  int sum(0);

  {
    const auto producer([&queue, &delay]()
    {
      for (int i(1); i <= n; ++i)
      {
        queue.push(i);
        std::this_thread::sleep_for(i * delay);
      }
    });

    const auto consumer([&queue, &sum]()
    {
      for (int j(0); j < n; ++j)
        sum += queue.pop();
    });

    std::jthread producer_thread(producer);
    std::jthread consumer_thread(consumer);
  }

  const int expected_sum(n * (n + 1) / 2);
  CHECK(sum == expected_sum);

  CHECK(queue.empty());
}

TEST_CASE("pop 2")
{
  using namespace ultra;

  ts_queue<int> queue;
  const std::vector delays =
    {
      std::chrono::milliseconds{100},
      std::chrono::milliseconds{200},
      std::chrono::milliseconds{300}
    };

  const int n(10);
  int sum(0);

  const int expected_sum(n * (n + 1) / 2 * delays.size());

  {
    std::vector<std::jthread> producer_threads;

    for (auto delay : delays)
    {
      const auto producer([&queue, delay]()
      {
        for (int j(1); j <= n; ++j)
        {
          queue.push(j);
          std::this_thread::sleep_for(delay);
        }
      });

      producer_threads.push_back(std::jthread(producer));
    }

    const auto consumer([&queue, &sum, expected_sum]()
    {
      while (sum < expected_sum)  // fully consume
        sum += queue.pop();
    });

    std::jthread consumer_thread(consumer);
  }

  CHECK(sum == expected_sum);
  CHECK(queue.empty());
}

}  // TEST_SUITE("ts_queue")
