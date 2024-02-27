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

#include <mutex>
#include <shared_mutex>

#if !defined(ULTRA_MUTEX_GUARDED_H)
#define      ULTRA_MUTEX_GUARDED_H

namespace ultra
{

///
/// A bundled mutex and `T`.
///
/// You cannot access `T` directly. The only way we’ll let you access `T` is by
/// calling `read` / `write` and passing it a callback that takes a `T &`
/// parameter.
///
/// When called, `read` / `write` locks the mutex, then invokes the callback
/// and finally unlocks the mutex again before returning.
///
/// This solves the issue of someone forgetting to lock the mutex before
/// accessing the field.
/// Since the mutex and field have been combined into a single variable, you no
/// longer have to be aware of the relationship between the two: it's been
/// encoded into the type system!
///
/// \see https://www.reddit.com/r/cpp/comments/p132c7/comment/h8b8nml/
///
template<class T,
         class M = std::mutex,
         template<class...> class WL = std::unique_lock,
         template<class...> class RL = std::unique_lock>
class mutex_guarded
{
public:
  mutex_guarded() = default;

  explicit mutex_guarded(T in) : val_(std::move(in)) {}

  mutex_guarded(const mutex_guarded &other)
  {
    if (this != &other)
      other.read([this](const T &other_val) { val_ = other_val; });
  }

  mutex_guarded &operator=(T in)
  {
    write([in = std::move(in)](auto &val) { val = in; });
    return *this;
  }

  auto read(auto f) const
  {
    auto l(lock());
    return f(val_);
  }

  auto write(auto f)
  {
    auto l(lock());
    return f(val_);
  }

private:
  mutable M mutex_ {};
  T val_;

  auto lock() const { return RL<M>(mutex_); }
  auto lock() { return WL<M>(mutex_); }
};

}  // namespace ultra

#endif  // include guard
