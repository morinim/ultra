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
/// - Jakob Progsch's simple and elegant thread pool
///   (https://github.com/progschj/ThreadPool);
/// - Paul T's performant thread pool
///   (https://github.com/DeveloperPaul123/thread-pool). We have also adapted
///   part of the test cases.
///
/// It allows scheduling tasks and retrieving results via futures (`submit`) or
/// simply executing tasks (`execute`).
///
/// \warning
/// When using a recursive approach to submit tasks, you must call the `wait`
/// member function to ensure that all tasks have been submitted before the
/// thread_pool destructor is called. Otherwise, the destructor may set the
/// non-accepting state prematurely (see the "Recursive execute calls work
/// correctly" test case).
///
class thread_pool
{
public:
  /// Constructor for the thread pool.
  ///
  /// \param[in] thread_count number of worker threads to launch. Defaults to
  ///                         the number of hardware threads available (or `1`
  ///                         if that value is not available)
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
            for (;;)
            {
              std::packaged_task<void()> task;

              {
                std::unique_lock lock(mutex_);
                cv_available_.wait(lock, stop_token,
                                   [this] { return !tasks_.empty(); });

                if (stop_token.stop_requested() && tasks_.empty())
                  return;

                task = std::move(tasks_.front());
                tasks_.pop();
              }

              task();

              if (std::lock_guard lock(task_counter_mutex_);
                  --task_counter_ == 0)
                cv_finished_.notify_all();
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
    if (!accepting_tasks_)
      throw std::runtime_error("submit called on stopped thread_pool");

    increment_task_counter();

    using return_type = std::invoke_result_t<std::decay_t<F>,
                                             std::decay_t<Args>...>;
    std::packaged_task<return_type()> task(
      [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      {
        return f(std::forward<Args>(args)...);
      });

    auto res(task.get_future());
    enqueue_task(std::move(task));
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
    if (!accepting_tasks_)
      throw std::runtime_error("execute called on stopped thread_pool");

    increment_task_counter();

    std::packaged_task<void()> task(
      [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      {
        f(std::forward<Args>(args)...);
      });

    enqueue_task(std::move(task));
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
    std::lock_guard lock(task_counter_mutex_);
    return task_counter_;
  }

  /// Blocks until all tasks have been completed.
  void wait()
  {
    std::unique_lock lock(task_counter_mutex_);
    cv_finished_.wait(lock, [this]{ return task_counter_ == 0; });
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

    cv_available_.notify_all();
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
  template <class Task>void enqueue_task(Task &&task)
  {
    {
      std::lock_guard lock(mutex_);
      tasks_.emplace(std::forward<Task>(task));
    }
    cv_available_.notify_one();
  }

  void increment_task_counter()
  {
    std::lock_guard lock(task_counter_mutex_);
    ++task_counter_;
  }

  mutable std::mutex mutex_;  // protects tasks_ and condition variables

  // A `stop_token` requires additional synchronization which is provided only
  // by the `wait` method of `std::condition_variable_any`.
  std::condition_variable_any cv_available_;  // notifies worker threads when
                                              // tasks are available

  // A separate mutex and condition variable to wait for task completion.
  mutable std::mutex task_counter_mutex_;
  std::condition_variable cv_finished_;
  std::size_t task_counter_ {0};

  std::atomic<bool> accepting_tasks_ {true};

  // `packaged_task<void()>` can store any callable, destroyable, movable
  // object that can be invoked with signature `void()`, which includes
  // `packaged_task<R()>`! It has an `operator() with signature `void()` since
  // the return value comes out of a `std::future`
  // (e.g see https://stackoverflow.com/a/31078143/3235496).
  std::queue<std::packaged_task<void()>> tasks_;

  // Placing the thread-related object at the end helps ensure that when other
  // data member are being destroyed, no thread will attempt to access a
  // container or mutex that has already been destroyed.
  std::vector<std::jthread> workers_;
};  // class thread_pool

}  // namespace ultra

#endif // include guard
