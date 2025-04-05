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

#if !defined(ULTRA_THREAD_POOL_H)
#define      ULTRA_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace ultra
{

/// A simple header-only thread pool implementation.
///
/// This thread pool is inspired by "C++ Concurrency in Action" by Anthony
/// Williams.
/// It allows scheduling tasks and retrieving results via futures (`submit`) or
/// simply executing tasks (`execute`).
class thread_pool
{
public:
  /// Constructor for the thread pool.
  ///
  /// \param[in] thread_count number of working threads to launch. Defaults to
  ///                         the hardware concurrency (or `1` if not available)
  explicit thread_pool(
    std::size_t thread_count = std::jthread::hardware_concurrency())
  {
    if (!thread_count)
      thread_count = 1;  // ensure at least one thread

    workers_.reserve(thread_count);

    try
    {
      while (thread_count--)
        workers_.emplace_back(
          [this]
          {
            for(;;)
            {
              std::packaged_task<void()> task;

              {
                std::unique_lock lock(mutex_);
                condition_.wait(lock,
                                [this]{ return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty())
                  return;

                task = std::move(tasks_.front());
                tasks_.pop();
              }

              task();
              --task_counter_;
            }
          });
    }
    catch (...)
    {
      // If thread creation fails, stop all threads gracefully before
      // rethrowing.
      shutdown();

      throw;
    }
  }

  /// Submits a task to the thread pool.
  ///
  /// \tparam F    type of the callable
  /// \tparam Args argument types to be forwarded to the callable
  /// \return      a future holding the result of the task
  ///
  /// \remark
  /// Throws `std::runtime_error` if submit is called after the pool is
  /// stopped.
  template<class F, class... Args>
  requires std::invocable<F, Args...>
  auto submit(F &&f, Args&&... args)
  {
    using return_type = std::invoke_result_t<std::decay_t<F>,
                                             std::decay_t<Args>...>;
    std::packaged_task<return_type()> task(
      [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      {
        return f(std::forward<Args>(args)...);
      });

    auto res(task.get_future());

    {
      std::lock_guard lock(mutex_);

      if (stop_)
        throw std::runtime_error("submit called on stopped thread_pool");

      tasks_.emplace(std::move(task));
    }

    ++task_counter_;
    condition_.notify_one();
    return res;
  }

  /// Executes a task without returning a future.
  ///
  /// \tparam F    type of the callable
  /// \tparam Args argument types to be forwarded to the callable
  ///
  /// \note
  /// Suitable for fire-and-forget tasks.
  ///
  /// \remark
  /// Throws `std::runtime_error` if execute is called after the pool is
  /// stopped.
  template<class F, class... Args>
  requires std::invocable<F, Args...>
  void execute(F &&f, Args&&... args)
  {
    // Using `std::function` could be more lightweight than
    // `std::packaged_task`.
    std::function<void()> task(
      [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      {
        f(std::forward<Args>(args)...);
      });

    {
      std::lock_guard lock(mutex_);

      if (stop_)
        throw std::runtime_error("submit called on stopped thread_pool");

      tasks_.emplace(std::move(task));
    }

    ++task_counter_;
    condition_.notify_one();
  }

  /// Gets the number of worker threads in the pool.
  ///
  /// \return the capacity of the thread pool
  [[nodiscard]] std::size_t capacity() const noexcept {return workers_.size();}

  /// Gets the current number of tasks in the queue.
  ///
  /// \return the number of queued tasks
  [[nodiscard]] std::size_t queue_size() const
  {
    std::lock_guard l(mutex_);
    return tasks_.size();
  }

  /// Checks whether there are unfinished tasks.
  ///
  /// \return `true` only if no tasks are currently being executed or queued
  [[nodiscard]] bool has_pending_tasks() const noexcept
  {
    return task_counter_;
  }

  /// Allows controlled early termination.
  void shutdown()
  {
    std::lock_guard lock(mutex_);
    stop_ = true;

    // Signal each already created thread to stop (using std::jthread, they
    // are expected to stop on destruction, but explicitly requesting stop
    // can speed up shutdown).
    for (auto &worker : workers_)
      worker.request_stop();
  }

  /// Stops all threads and cleans up.
  ///
  /// Signals the threads to stop and notifies all waiting threads.
  ~thread_pool()
  {
    shutdown();
    condition_.notify_all();
  }

  /// thread_pool is non-copyable.
  thread_pool(const thread_pool &) = delete;
  thread_pool &operator=(const thread_pool &) = delete;

private:
  // Note that the order of declaration of the members is important: both the
  // `stop_` flag and the `tasks_` queue must be declared before the threads
  // vector. This ensures that the members are destroyed in the right order:
  // you can't destroy the queue safely until all the threads have stopped.

  mutable std::mutex mutex_;  // protects tasks_ and stop_
  std::condition_variable condition_;
  bool stop_ {false};

  std::queue<std::packaged_task<void()>> tasks_;
  std::vector<std::jthread> workers_;

  std::atomic<std::size_t> task_counter_ {0};
};  // class thread_pool

}  // namespace ultra

#endif // include guard
