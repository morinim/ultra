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

#include <chrono>
#include <random>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

using namespace std::chrono_literals;

TEST_SUITE("thread_pool")
{

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

  CHECK(!pool.has_pending_tasks());
  CHECK(pool.queue_size() == 0);
  CHECK(result == TASK_COUNT);
  CHECK(thread_ids.size() == THREAD_COUNT);
}

TEST_CASE("Miscellaneous tasks")
{
  ultra::thread_pool pool(2);

  constexpr int MAGIC_NUMBER = 42;
  auto fi(pool.submit([] { return MAGIC_NUMBER; }));
  auto fs(pool.submit([] { return std::string{"42"}; }));

  CHECK(fi.get() == MAGIC_NUMBER);
  CHECK(fs.get() == std::string{"42"});
}

TEST_CASE("Lambdas")
{
  const unsigned TASK_COUNT(4);
  std::vector<std::future<unsigned>> v;

  ultra::thread_pool pool(4);

  for (unsigned i(0); i < TASK_COUNT; ++i)
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
  std::atomic<unsigned> counter(0);
  constexpr unsigned TOTAL_TASKS(30);
  {
    ultra::thread_pool pool(4);
    for (unsigned i(0); i < TOTAL_TASKS; ++i)
    {
      const auto task([i, &counter]
      {
        std::this_thread::sleep_for(
          std::chrono::milliseconds((i + 1) * 10));

        ++counter;
      });

      pool.execute(task);
    }
  }

  CHECK(counter == TOTAL_TASKS);
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

TEST_CASE("Ensure task exception doesn't kill worker thread")
{
  const auto throw_task([](int) -> int
  {
    throw std::logic_error("Error occurred.");
  });

  const auto regular_task([](int input) -> int { return input * 2; });

  std::atomic_uint_fast64_t count(0);

  const auto throw_no_return([]
  {
    throw std::logic_error("Error occurred.");
  });

  const auto no_throw_no_return([&count]
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    count += 1;
  });

  {
    ultra::thread_pool pool{};

    auto throw_future(pool.submit(throw_task, 1));
    auto no_throw_future(pool.submit(regular_task, 2));

    CHECK_THROWS(throw_future.get());
    CHECK(no_throw_future.get() == 4);

    // Do similar check for tasks without return.
    pool.execute(throw_no_return);
    pool.execute(no_throw_no_return);
  }

  CHECK(count == 1);
}

TEST_CASE("Ensure work completes when one thread is running, another is "
          "finished, and a new task is submitted")
{
  std::atomic<std::size_t> last_thread;

  {
    ultra::thread_pool thread_pool(2);

    // Ties up the first thread.
    thread_pool.execute([&last_thread]
    {
      std::this_thread::sleep_for(5s);
      last_thread = 1;
    });

    // Runs a quick job on the second thread.
    thread_pool.execute([&last_thread]
    {
      std::this_thread::sleep_for(50ms);
      last_thread = 2;
    });

    // Waits for the second thread to finish.
    std::this_thread::sleep_for(1s);

    // Executes a quick job.
    thread_pool.execute([&last_thread]
    {
      std::this_thread::sleep_for(50ms);
      last_thread = 3;
    });
  }

  CHECK(last_thread == 1);
}

void recursive_sequential_sum(std::atomic<int> &counter, int count,
                              ultra::thread_pool &pool)
{
  counter += count;

  if (count > 1)
  {
    pool.submit(recursive_sequential_sum,
                 std::ref(counter), count - 1, std::ref(pool));
  }
}

TEST_CASE("Recursive execute calls work correctly")
{
  std::atomic<int> counter(0);

  constexpr auto start(1000);
  {
    ultra::thread_pool pool(4);
    recursive_sequential_sum(counter, start, pool);

    pool.wait();
  }

  auto expected_sum(0);
  for (int i(0); i <= start; ++i)
    expected_sum += i;

  CHECK(expected_sum == counter);
}

void recursive_parallel_sort(int *begin, int *end, int split_level,
                             ultra::thread_pool &pool)
{
  if (split_level < 2 || end - begin < 2)
    std::sort(begin, end);
  else
  {
    const auto mid(begin + (end - begin) / 2);

    if (split_level == 2)
    {
      const auto future(
        pool.submit(recursive_parallel_sort, begin, mid, split_level/2,
                    std::ref(pool)));
      std::sort(mid, end);
      future.wait();
    }
    else
    {
      const auto left(
        pool.submit(recursive_parallel_sort, begin, mid, split_level/2,
                    std::ref(pool)));
      const auto right(
        pool.submit(recursive_parallel_sort, mid, end, split_level/2,
                    std::ref(pool)));

      left.wait();
      right.wait();
    }
    std::inplace_merge(begin, mid, end);
  }
}

TEST_CASE("Recursive parallel sort")
{
  std::vector<int> data(10000);
  std::iota(data.begin(), data.end(), 0);

  std::ranges::shuffle(data, std::mt19937{std::random_device{}()});

  {
    ultra::thread_pool pool(4);
    recursive_parallel_sort(data.data(), data.data() + data.size(), 4, pool);

    pool.wait();
  }

  CHECK(std::ranges::is_sorted(data));
}

TEST_CASE("Ensure wait() properly blocks current execution.")
{
  std::atomic counter(0);
  unsigned total_tasks(0);
  constexpr unsigned THREAD_COUNT(4);

  SUBCASE("with tasks") { total_tasks = 30; }
  SUBCASE("with no tasks") { total_tasks = 0; }
  SUBCASE("task count < thread count") { total_tasks = THREAD_COUNT / 2; }

  ultra::thread_pool pool(THREAD_COUNT);
  for (unsigned i(0); i < total_tasks; ++i)
  {
    const auto task([i, &counter]
    {
      std::this_thread::sleep_for(std::chrono::milliseconds((i + 1) * 10));
      ++counter;
    });
    pool.execute(task);
  }

  pool.wait();

  CHECK(counter == total_tasks);
}

struct counter_wrapper
{
  void increment_counter() {counter.fetch_add(1, std::memory_order_release);}

  std::atomic<unsigned> counter {0};
};

TEST_CASE("Ensure wait() properly waits for tasks to fully complete")
{
  ultra::thread_pool local_pool;
  constexpr unsigned task_count(10);

  std::array<unsigned, task_count> counts = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  for (unsigned i(0); i < task_count; ++i)
  {
    counter_wrapper cnt_wrp;

    for (unsigned var1(0); var1 < 17; ++var1)
      for (unsigned var2(0); var2 < 12; ++var2)
        local_pool.execute([&cnt_wrp] { cnt_wrp.increment_counter(); });

    local_pool.wait();

    counts[i] = cnt_wrp.counter.load(std::memory_order_acquire);
  }

  const auto all_correct_count(
    std::ranges::all_of(counts, [](int count) { return count == 17 * 12; }));

  const unsigned sum(std::accumulate(counts.begin(), counts.end(), 0u));

  CHECK(sum == 17 * 12 * task_count);
  CHECK(all_correct_count);
}

TEST_CASE("Ensure wait() can be called multiple times on the same pool")
{
  ultra::thread_pool local_pool;
  constexpr unsigned task_count(10);

  std::array<unsigned, task_count> counts = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (unsigned i(0); i < task_count; ++i)
  {
    counter_wrapper cnt_wrp;

    for (unsigned var1(0); var1 < 16; ++var1)
      for (unsigned var2(0); var2 < 13; ++var2)
        local_pool.execute([&cnt_wrp] { cnt_wrp.increment_counter(); });

    local_pool.wait();
    counts[i] = cnt_wrp.counter.load(std::memory_order_acquire);
  }

  auto all_correct_count(
    std::ranges::all_of(counts, [](unsigned count) {return count == 16*13;}));

  auto sum(std::accumulate(counts.begin(), counts.end(), 0u));

  CHECK(sum == 16 * 13 * task_count);
  CHECK(all_correct_count);

  for (unsigned i(0); i < task_count; ++i)
  {
    counter_wrapper cnt_wrp;

    for (unsigned var1(0); var1 < 17; ++var1)
      for (unsigned var2(0); var2 < 12; ++var2)
        local_pool.execute([&cnt_wrp] { cnt_wrp.increment_counter(); });

    local_pool.wait();
    counts[i] = cnt_wrp.counter.load(std::memory_order_acquire);
  }

  all_correct_count = std::ranges::all_of(counts,
                                          [](unsigned count)
                                          { return count == 17 * 12; });
  sum = std::accumulate(counts.begin(), counts.end(), 0u);
  CHECK(sum == 17 * 12 * task_count);
  CHECK(all_correct_count);
}

}  // TEST_SUITE("thread_pool")
