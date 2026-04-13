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

#include "utility/ts_queue.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <chrono>
#include <thread>
#include <vector>

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
      for (int j(0); j < n;)
        if (auto val = queue.try_pop())
        {
          sum += *val;
          ++j;
        }
        else
          std::this_thread::yield();
    });

    std::jthread producer_thread(producer);
    std::jthread consumer_thread(consumer);
  }

  const int expected_sum(n * (n + 1) / 2);
  CHECK(sum == expected_sum);
  CHECK(queue.empty());
}


TEST_CASE("try_pop on empty queue")
{
  using namespace ultra;

  ts_queue<int> queue;
  CHECK(!queue.try_pop().has_value());
  CHECK(queue.empty());
  CHECK(queue.size() == 0);
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
      std::chrono::milliseconds{1},
      std::chrono::milliseconds{2},
      std::chrono::milliseconds{3}
    };

  const int n(10);
  const int producer_count(static_cast<int>(delays.size()));
  const int total_items(n * producer_count);
  const int expected_sum(n * (n + 1) / 2 * producer_count);
  int sum(0);

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

      producer_threads.emplace_back(producer);
    }

    const auto consumer([&queue, &sum, total_items]()
    {
      for (int i(0); i < total_items; ++i)
        sum += queue.pop();
    });

    std::jthread consumer_thread(consumer);
  }

  CHECK(sum == expected_sum);
  CHECK(queue.empty());
}

TEST_CASE("move-only type")
{
  using namespace ultra;

  ts_queue<std::unique_ptr<int>> queue;
  queue.push(std::make_unique<int>(42));

  auto val = queue.pop();
  CHECK(*val == 42);
  CHECK(queue.empty());
}

}  // TEST_SUITE
