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

#include "utility/thread_pool.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("thread_pool")
{

using namespace std::chrono_literals;

TEST_CASE("Number of threads")
{
  SUBCASE("Default")
  {
    ultra::thread_pool pool;
    CHECK(pool.capacity() == std::thread::hardware_concurrency());
  }

  SUBCASE("hardware_concurrency == 0")
  {
    ultra::thread_pool pool(0);
    CHECK(pool.capacity() == 1);
  }
}

TEST_CASE("Run more tasks than threads")
{
  const std::size_t THREAD_COUNT(2);
  const std::size_t TASK_COUNT(20);

  std::mutex mutex;
  std::size_t result(0);
  std::set<std::thread::id> thread_ids;

  ultra::thread_pool pool(THREAD_COUNT);
  std::vector<std::future<void>> futures;

  for (std::size_t i(0); i < TASK_COUNT; ++i)
    futures.push_back(pool.submit([&]
    {
      std::this_thread::sleep_for(1ms);
      std::lock_guard<std::mutex> l(mutex);
      ++result;
      thread_ids.insert(std::this_thread::get_id());
    }));

  for (auto &f : futures)
    f.wait();

  CHECK(pool.queue_size() == 0);
  CHECK(result == TASK_COUNT);
  CHECK(thread_ids.size() == THREAD_COUNT);
}

TEST_CASE("Miscellaneous tasks")
{
  ultra::thread_pool pool(2);

  constexpr int MAGIC_NUMBER = 42;
  auto fi = pool.submit([] { return MAGIC_NUMBER; });
  auto fs = pool.submit([] { return std::string{"42"}; });

  CHECK(fi.get() == MAGIC_NUMBER);
  CHECK(fs.get() == std::string{"42"});
}

TEST_CASE("Lambdas")
{
  const std::size_t TASK_COUNT(4);
  std::vector<std::future<std::size_t>> v;

  ultra::thread_pool pool(4);

  for (std::size_t i(0); i < TASK_COUNT; ++i)
  {
    v.push_back(pool.submit([task_num = i]
    {
      std::this_thread::sleep_for(1ms);
      return task_num;
    }));
  }

  for (std::size_t i(0); i < TASK_COUNT; ++i)
    CHECK(i == v[i].get());

  CHECK(pool.queue_size() == 0);
}

TEST_CASE("Exception")
{
  ultra::thread_pool pool(1);
  auto f = pool.submit([] { throw std::runtime_error{"Error"}; });

  CHECK_THROWS_AS(f.get(), std::runtime_error);
}

TEST_CASE("Capacity")
{
  ultra::thread_pool pool(4);
  CHECK(pool.capacity() == 4);
}

TEST_CASE("Empty queue")
{
  ultra::thread_pool pool(4);
  std::this_thread::sleep_for(1s);
}

auto sum(int a, int b) { return a + b; }

TEST_CASE("Sum function")
{
  ultra::thread_pool pool;

  SUBCASE("Functor")
  {
    auto f(pool.submit(std::plus{}, 2, 2));
    CHECK(f.get() == 4);
  }

  SUBCASE("Global function")
  {
    auto f(pool.submit(sum, 2, 2));
    CHECK(f.get() == 4);
  }

  SUBCASE("Lambda")
  {
    auto f(pool.submit([](int a, int b) {return a + b; }, 2, 2));
    CHECK(f.get() == 4);
  }
}

TEST_CASE("Passing a reference")
{
  int x(2);

  SUBCASE("submit")
  {
    {
      ultra::thread_pool pool;
      pool.submit([](int &a) { a *= 2; }, std::ref(x));
    }

    CHECK(x == 4);
  }

  SUBCASE("execute")
  {
    {
      ultra::thread_pool pool;
      pool.execute([](int &a) { a *= 2; }, std::ref(x));
    }

    CHECK(x == 4);
  }
}

TEST_CASE("Ensure input params are properly passed")
{
  ultra::thread_pool pool(4);
  constexpr auto TOTAL_TASKS(30);

  std::vector<std::future<int>> futures;

  for (auto i(0); i < TOTAL_TASKS; ++i)
  {
    const auto task([index = i]() { return index; });

    futures.push_back(pool.submit(task));
  }

  for (auto j(0); j < TOTAL_TASKS; ++j)
    CHECK(j == futures[j].get());
}

TEST_CASE("Support params of different types")
{
  ultra::thread_pool pool;

  struct test_struct
  {
    int value {};
    double d_value {};
  } test;

  const auto task(
    [&test](int x, double y)
    {
      test.value = x;
      test.d_value = y;

      return test_struct{x, y};
    });

  auto future(pool.submit(task, 2, 3.2));
  const auto result(future.get());
  CHECK(result.value == test.value);
  CHECK(result.d_value == doctest::Approx(test.d_value));
}

TEST_CASE("Ensure work completes upon destruction")
{
  std::atomic<int> counter;
  constexpr auto TOTAL_TASKS(30);
  {
    ultra::thread_pool pool(4);
    for (auto i(0); i < TOTAL_TASKS; ++i)
    {
      auto task([i, &counter]()
      {
        std::this_thread::sleep_for(
          std::chrono::milliseconds((i + 1) * 10));

        ++counter;
      });

      pool.execute(task);
    }
  }

  CHECK(counter.load() == TOTAL_TASKS);
}

static std::thread::id test_function(unsigned delay)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(delay + 1));
  return std::this_thread::get_id();
}

TEST_CASE("Threads are reused")
{
  const size_t THREAD_COUNT(4);
  ultra::thread_pool pool(THREAD_COUNT);
  std::vector<std::future<std::thread::id>> futures;
  std::set<std::thread::id> thread_ids;

  for (std::size_t i(0); i < THREAD_COUNT; ++i)
    futures.push_back(pool.submit(test_function, i));

  for (std::size_t i(0); i < THREAD_COUNT; ++i)
  {
    auto r(futures[i].get());
    auto iter(thread_ids.insert(r));

    // New thread is used.
    CHECK(iter.second);
  }

  futures.clear();

  for (std::size_t i(0); i < THREAD_COUNT; ++i)
    futures.push_back(pool.submit(test_function, i));

  for (std::size_t i(0); i < THREAD_COUNT; ++i)
  {
    auto r(futures[i].get());
    auto iter(thread_ids.find(r));

    CHECK(iter != thread_ids.end());
    thread_ids.erase(iter);
  }
}

}  // TEST_SUITE("thread_pool")
