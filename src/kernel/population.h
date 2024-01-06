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

#if !defined(ULTRA_POPULATION_H)
#define      ULTRA_POPULATION_H

#include <algorithm>
#include <numeric>
#include <shared_mutex>

#include "kernel/environment.h"
#include "kernel/individual.h"
#include "kernel/problem.h"
#include "kernel/random.h"

#include "utility/log.h"

namespace ultra
{

template<class P>
concept SizedRangeOfIndividuals = requires(const P &p)
{
  requires std::ranges::sized_range<P>;
  requires Individual<std::ranges::range_value_t<P>>;
};

template<class P>
concept Population = requires(const P &p)
{
  requires SizedRangeOfIndividuals<P>;
  typename P::value_type;
  requires Individual<typename P::value_type>;

  typename P::coord;
  p[typename P::coord()];
};

template<class P>
concept PopulationWithMutex = Population<P> && requires(const P &p)
{
  p.mutex();
};

///
/// A group of individuals which may interact together (for example by mating)
/// producing offspring.
///
/// Typical population size in GP ranges from ten to many thousands. The
/// population is organized in one or more layers that can interact in
/// many ways (depending on the evolution strategy).
///
template<Individual I>
class population
{
public:
  struct coord;
  class layer_t;

  using value_type = I;
  using difference_type = std::ptrdiff_t;

  explicit population(const ultra::problem &);

  [[nodiscard]] std::size_t layers() const;
  [[nodiscard]] const layer_t &layer(std::size_t) const;
  [[nodiscard]] layer_t &layer(std::size_t);

  void init(layer_t &);
  void add_layer();
  void remove(layer_t &);

  [[nodiscard]] I &operator[](const coord &);
  [[nodiscard]] const I &operator[](const coord &) const;

  [[nodiscard]] std::size_t size() const;

  void inc_age();

  [[nodiscard]] const ultra::problem &problem() const;

  // Iterators.
  template<bool> class base_iterator;
  using const_iterator = base_iterator<true>;
  using iterator = base_iterator<false>;

  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const;
  [[nodiscard]] iterator begin();
  [[nodiscard]] iterator end();

  [[nodiscard]] bool is_valid() const;

  // Serialization.
  [[nodiscard]] bool load(std::istream &);
  [[nodiscard]] bool save(std::ostream &) const;

private:
  const ultra::problem *prob_;

  std::vector<layer_t> layers_ {};
};

template<Individual I>
[[nodiscard]] population<I> make_debug_population(const ultra::problem &);

namespace random
{
template<Individual I>
[[nodiscard]] typename population<I>::coord coord(const population<I> &);

template<Individual I>
[[nodiscard]] typename population<I>::coord coord(
  const population<I> &, typename population<I>::coord, std::size_t);

template<PopulationWithMutex P>
[[nodiscard]] typename P::value_type individual(const P &);
}

template<Individual I>
class population<I>::layer_t
{
public:
  using value_type = I;
  using const_iterator = typename std::vector<I>::const_iterator;
  using iterator = typename std::vector<I>::iterator;
  using difference_type = typename std::vector<I>::difference_type;
  using coord = std::size_t;

  [[nodiscard]] I &operator[](std::size_t);
  [[nodiscard]] const I &operator[](std::size_t) const;

  [[nodiscard]] std::size_t size() const;

  [[nodiscard]] bool empty() const;
  void clear();

  [[nodiscard]] std::size_t allowed() const;
  void allowed(std::size_t);

  [[nodiscard]] unsigned max_age() const;
  void max_age(unsigned);

  void push_back(const I &);
  void pop_back();

  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] std::shared_mutex &mutex() const;

private:
  mutable std::shared_ptr<std::shared_mutex> pmutex_
  {std::make_shared<std::shared_mutex>()};

  std::vector<I> members_ {};
  std::size_t allowed_ {0};

  unsigned max_age_ {std::numeric_limits<unsigned>::max()};
};

namespace random
{
template<Population P> [[nodiscard]] std::size_t coord(const P &);
template<Population P>
[[nodiscard]] std::size_t coord(const P &, std::size_t, std::size_t);
}

#include "kernel/population.tcc"
#include "kernel/population_coord.tcc"
#include "kernel/population_iterator.tcc"

}  // namespace ultra

#endif  // include guard
