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

#include <algorithm>
#include <mutex>
#include <numeric>
#include <shared_mutex>

#include "kernel/population.h"
#include "kernel/problem.h"
#include "kernel/random.h"

#include "utility/misc.h"

namespace ultra
{

///
/// A group of individuals which may interact together (for example by mating)
/// producing offspring.
///
/// Typical population size ranges from ten to many thousands. This type of
/// population is organized in one layer.
///
template<Individual I>
class linear_population
{
public:
  using value_type = I;
  using const_iterator = typename std::vector<I>::const_iterator;
  using iterator = typename std::vector<I>::iterator;
  using difference_type = typename std::vector<I>::difference_type;
  using coord = std::size_t;

  linear_population() = default;
  explicit linear_population(const ultra::problem &);

  [[nodiscard]] I &operator[](std::size_t);
  [[nodiscard]] const I &operator[](std::size_t) const;

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t safe_size() const;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] std::size_t allowed() const;
  void allowed(std::size_t);

  [[nodiscard]] unsigned max_age() const noexcept;
  void max_age(unsigned) noexcept;

  [[nodiscard]] population_uid uid() const noexcept;

  void clear() noexcept;

  void push_back(const I &);
  void pop_back();

  // Iterators.
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator end() const;
  [[nodiscard]] iterator end();

  // Misc.
  void reset(const ultra::problem &);
  void inc_age();

  [[nodiscard]] std::shared_mutex &mutex() const;

  // Serialization.
  [[nodiscard]] bool load(std::istream &, const symbol_set &);
  [[nodiscard]] bool save(std::ostream &) const;

  [[nodiscard]] bool is_valid() const;

private:
  mutable ignore_copy<std::shared_mutex> mutex_ {};

  std::vector<I> members_ {};

  std::size_t allowed_ {std::numeric_limits<difference_type>::max()};
  std::size_t min_allowed_ {1};

  unsigned max_age_ {std::numeric_limits<unsigned>::max()};

  ignore_copy<app_level_uid> uid_ {};
};

#include "kernel/linear_population.tcc"

}  // namespace ultra

#endif  // include guard
