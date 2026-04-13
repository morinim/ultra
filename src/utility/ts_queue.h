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

#if !defined(ULTRA_TS_QUEUE_H)
#define      ULTRA_TS_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace ultra
{

///
/// A thread-safe FIFO queue.
///
/// \tparam T type of the stored elements
///
/// `ts_queue` provides synchronized access to a `std::queue`, allowing
/// multiple threads to enqueue and dequeue elements concurrently.
///
/// The class offers two ways to remove elements:
/// - `pop()` blocks until an element becomes available;
/// - `try_pop()` returns immediately and reports failure via `std::optional`.
///
/// \warning
/// This class does **not** provide a shutdown or close mechanism. A thread
/// blocked in `pop()` remains blocked until another thread pushes an element.
/// Users must therefore ensure that no thread waits on a queue that will never
/// receive further elements.
///
template <class T>
class ts_queue
{
public:
  // ---- Member types ----
  using container_type = std::queue<T>;
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
  /// \param[in] item element to be inserted
  ///
  /// After the new element has been enqueued, one waiting consumer is
  /// notified.
  void push(T item)
  {
    {
      std::lock_guard lock(mutex_);
      queue_.push(std::move(item));
    }

    cond_.notify_one();
  }

  /// Constructs an element in-place at the back of the queue.
  ///
  /// \param[in] args arguments to forward to the constructor of `T`
  ///
  /// The element is constructed directly in the underlying container.
  ///
  /// After the new element has been enqueued, one waiting consumer is
  /// notified.
  template<class... Args>
  void emplace(Args &&...args)
  {
    {
      std::lock_guard lock(mutex_);
      queue_.emplace(std::forward<Args>(args)...);
    }

    cond_.notify_one();
  }

  /// Removes and returns the element at the front of the queue.
  ///
  /// \return the first available element
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

  /// Tries to remove and return the element at the front of the queue.
  ///
  /// \return the first available element, or `std::nullopt` if the queue is
  ///         empty
  ///
  /// \remark
  /// Unlike `pop()`, this function never blocks.
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
  /// The returned value is only a snapshot. In a concurrent environment,
  /// another thread may modify the queue immediately after this function
  /// returns. In particular, it must not be used to decide whether a
  /// subsequent call to `pop()` can proceed safely.
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
  /// The returned value is only a snapshot. In a concurrent environment,
  /// another thread may modify the queue immediately after this function
  /// returns. In particular, it must not be used to decide whether a
  /// subsequent call to `pop()` can proceed safely.
  ///
  [[nodiscard]] size_type size() const
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
