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
/// This thread pool is inspired by:
/// - "C++ Concurrency in Action" by Anthony Williams;
/// - Jakob Progsch's simple end elegant thread pool
///   (https://github.com/progschj/ThreadPool);
/// - Paul T's performant thread pool
///   (https://github.com/DeveloperPaul123/thread-pool). We have also adapted
///   part of the test cases.
///
/// It allows scheduling tasks and retrieving results via futures (`submit`) or
/// simply executing tasks (`execute`).
class thread_pool
{
public:
  /// Constructor for the thread pool.
  ///
  /// \param[in] thread_count number of worker threads to launch. Defaults to
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
          [this](std::stop_token stop_token)
          {
            for(;;)
            {
              std::packaged_task<void()> task;

              {
                std::unique_lock lock(mutex_);
                condition_.wait(
                  lock,
                  [this, &stop_token]
                  {
                    return stop_token.stop_requested() || !tasks_.empty();
                  });

                if (stop_token.stop_requested() && tasks_.empty())
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
  /// \throws std::runtime_error if submit is called after the pool is stopped.
  template<class F, class... Args>
  requires std::invocable<F, Args...>
  auto submit(F &&f, Args&&... args)
  {
    ++task_counter_;

    if (!accepting_tasks_)
      throw std::runtime_error("submit called on stopped thread_pool");

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
      tasks_.emplace(std::move(task));
    }

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
  /// \throws std::runtime_error if execute is called after the pool is stopped.
  template<class F, class... Args>
  requires std::invocable<F, Args...>
  void execute(F &&f, Args&&... args)
  {
    ++task_counter_;

    if (!accepting_tasks_)
      throw std::runtime_error("execute called on stopped thread_pool");

    std::packaged_task<void()> task(
      [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      {
        f(std::forward<Args>(args)...);
      });

    {
      std::lock_guard lock(mutex_);
      tasks_.emplace(std::move(task));
    }

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

  /// \return `true` if there are unfinished tasks (i.e. if the task counter is
  ///                nonzero)
  [[nodiscard]] bool has_pending_tasks() const noexcept
  {
    return task_counter_;
  }

  /// Initiates the termination process.
  ///
  /// Shutdown prevents new submissions and signals worker threads to exit once
  /// the current tasks have been processed and there are no pending tasks,
  /// which lets the pool gracefully complete any tasks that were already
  /// queued.
  void shutdown()
  {
    accepting_tasks_ = false;

    for (auto &worker : workers_)
      worker.request_stop();

    condition_.notify_all();
  }

  /// Stops all threads and cleans up.
  ///
  /// Signals the threads to stop and notifies all waiting threads.
  ~thread_pool()
  {
    shutdown();
  }

  /// thread_pool is non-copyable.
  thread_pool(const thread_pool &) = delete;
  thread_pool &operator=(const thread_pool &) = delete;

private:
  mutable std::mutex mutex_;  // protects tasks_ and condition variables
  std::condition_variable condition_;  // notifies worker threads when tasks
                                       // are available
  std::atomic<bool> accepting_tasks_ {true};
  std::atomic<std::size_t> task_counter_ {0};

  std::queue<std::packaged_task<void()>> tasks_;

  // Placing the thread-related object at the end helps ensure that when other
  // data member are being destroyed, no thread will attempt to access a
  // container or mutex that has already been destroyed.
  std::vector<std::jthread> workers_;
};  // class thread_pool

}  // namespace ultra

#endif // include guard
