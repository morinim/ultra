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

#if !defined(ULTRA_LINEAR_POPULATION_H)
#define      ULTRA_LINEAR_POPULATION_H

#include "kernel/population.h"
#include "kernel/problem.h"
#include "kernel/random.h"

#include "utility/misc.h"

#include <algorithm>
#include <mutex>
#include <numeric>
#include <shared_mutex>

namespace ultra
{

///
/// A one-dimensional population of individuals.
///
/// \tparam I an individual type satisfying the `Individual` concept
///
/// A `linear_population` represents a collection of individuals organised as a
/// single, flat sequence. Individuals may interact (for example, by mating or
/// selection) to produce offspring.
///
/// The population enforces an upper bound (`allowed()`) and a lower bound on
/// its size. The invariant:
///
/// \code
///   min_allowed_ <= size() <= allowed()
/// \endcode
///
/// is maintained whenever the object is in a valid state.
///
/// ### Thread safety
///
/// This class does **not** provide internal synchronisation for most
/// operations.
/// Clients are responsible for serialising concurrent access using the mutex
/// returned by `mutex()`.
///
/// Multiple threads may safely call `const` member functions concurrently,
/// provided that no thread performs a modifying operation at the same time.
///
/// Mixing read-only access with modifiers, or invoking non-const member
/// functions concurrently, requires external synchronisation.
///
/// The only method that is explicitly thread-safe is `safe_size()`.
///
/// ### Copy semantics
///
/// Copying a population does not preserve its unique identifier (`uid()`),
/// which is generated per application instance.
///
template<Individual I>
class linear_population
{
public:
  // ---- Member types ----
  using value_type      = I;
  using const_iterator  = typename std::vector<I>::const_iterator;
  using iterator        = typename std::vector<I>::iterator;
  using difference_type = typename std::vector<I>::difference_type;
  using coord           = std::size_t;

  // ---- Constructors ----

  /// Constructs an empty population.
  ///
  /// The population is created with default limits and contains no
  /// individuals.
  linear_population() = default;
  explicit linear_population(const ultra::problem &);

  // ---- Element access ----
  [[nodiscard]] I &operator[](std::size_t);
  [[nodiscard]] const I &operator[](std::size_t) const;

  // ---- Capacity ----
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t safe_size() const;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] std::size_t allowed() const noexcept;
  void allowed(std::size_t);

  // ---- Age management ----
  [[nodiscard]] unsigned max_age() const noexcept;
  void max_age(unsigned) noexcept;
  void inc_age();

  // ---- Modifiers ----
  void clear() noexcept;

  void push_back(const I &);
  void pop_back();

  void reset(const ultra::problem &);

  // ---- Iterators ----
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;
  [[nodiscard]] iterator end() noexcept;

  // ---- Synchronisation ----
  [[nodiscard]] std::shared_mutex &mutex() const;

  // ---- Serialization ----
  [[nodiscard]] bool load(std::istream &, const symbol_set &);
  [[nodiscard]] bool save(std::ostream &) const;

  // ---- Identification and validation ----
  [[nodiscard]] population_uid uid() const noexcept;

  [[nodiscard]] bool is_valid() const;

private:
  mutable ignore_copy<std::shared_mutex> mutex_ {};

  std::vector<I> members_ {};

  std::size_t allowed_ {std::numeric_limits<std::size_t>::max()};
  std::size_t min_allowed_ {1};

  unsigned max_age_ {std::numeric_limits<unsigned>::max()};

  ignore_copy<app_level_uid> uid_ {};
};

#include "kernel/linear_population.tcc"

}  // namespace ultra

#endif  // include guard
