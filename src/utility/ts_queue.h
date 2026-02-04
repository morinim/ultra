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

#if !defined(ULTRA_TS_QUEUE_H)
#define      ULTRA_TS_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

namespace ultra
{

///
/// Allows multiple threads to aenqueue and dequeue elements concurrently.
///
///
///
template <class T>
class ts_queue
{
public:
  // ---- Member types ----
  using container_type = typename std::queue<T>;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;

  // ---- Constructors ----
  ts_queue() = default;
  ts_queue(const ts_queue &) = delete;
  ts_queue &operator=(const ts_queue &) = delete;

  // ---- Modifiers ----
  /// Pushes the given element to the end of the queue.
  ///
  /// \\param[in] value the value of the element to push
  void push(const T &item)
  {
    {
      std::lock_guard lock(mutex_);
      queue_.push(item);
    }

    cond_.notify_one();
  }

  /// Pushes a new element to the end of the queue.
  ///
  /// \param[in] args arguments to forward to the constructor of the element
  ///
  /// The element is constructed in-place, i.e. no copy or move operations are
  /// performed. The constructor of the element is called with exactly the same
  /// arguments as supplied to the function.
  template<class... Args>
  void emplace(Args &&...args)
  {
    {
      std::lock_guard lock(mutex_);
      queue_.emplace(std::forward<Args>(args)...);
    }

    cond_.notify_one();
  }

  /// Removes an element from the front of the queue.
  ///
  /// \return the first element
  ///
  /// \warning
  /// Blocks if queue is empty.
  T pop()
  {
    // `condition_variable` works only with `unique_lock`.
    std::unique_lock lock(mutex_);

    // Wait until queue is not empty.
    cond_.wait(lock, [this] { return !queue_.empty(); });

    // Retrieve item.
    T item(std::move(queue_.front()));
    queue_.pop();

    // Keeping item non-const, the return statement can use move semantics (or
    // elide copies) to return the value efficiently.
    return item;
  }

  /// Removes an element from the front of the queue.
  ///
  /// \return the first element if available, otherwise an empty object
  ///
  /// \remark
  /// Doesn't block if queue is empty.
  [[nodiscard]] std::optional<T> try_pop()
  {
    std::lock_guard lock(mutex_);

    if (queue_.empty())
      return {};

    // Retrieve item.
    T item(std::move(queue_.front()));
    queue_.pop();

    // Keeping item non-const, the return statement can use move semantics (or
    // elide copies) to return the value efficiently.
    return item;
  }

  // ---- Capacity ----
  /// Checks if the container has no elements.
  ///
  /// \return `true` if the container is empty, `false` otherwise
  ///
  /// \warning
  /// Should only be used in a multiple producer single consumer environment.
  /// In general the queue cannot guarantee that matters won't change between
  /// the time the client queries `empty()` and the time `pop()` is called,
  /// making the point entirely moot, and this code a potential source of
  /// intermittent (read: hard to pin) bugs:
  ///
  /// ```c++
  /// if (!queue.empty())
  /// {
  ///   // What could possibly go wrong? A lot, it turns out.
  ///   auto elem = queue.pop();
  /// }
  /// ```
  ///
  [[nodiscard]] bool empty() const
  {
    std::lock_guard lock(mutex_);
    return queue_.empty();
  }

  ///
  /// \return the number of elements in the container
  ///
  /// \warning
  /// Should only be used in a multiple producer single consumer environment.
  /// In general the queue cannot guarantee that matters won't change between
  /// the time the client queries `size()` and the time `pop()` is called,
  /// making the point entirely moot, and this code a potential source of
  /// intermittent (read: hard to pin) bugs:
  ///
  /// ```c++
  /// if (queue.size())
  /// {
  ///   // What could possibly go wrong? A lot, it turns out.
  ///   auto elem = queue.pop();
  /// }
  /// ```
  ///
  [[nodiscard]] std::size_t size() const
  {
    std::lock_guard lock(mutex_);
    return queue_.size();
  }

private:
  mutable std::mutex mutex_ {};      // for thread synchronization
  std::condition_variable cond_ {};  // for signaling
  std::queue<T> queue_ {};           // underlying queue
};  // class ts_queue

}  // namespace ultra

#endif // include guard
