/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
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

#include "kernel/environment.h"
#include "kernel/individual.h"
#include "kernel/problem.h"

#include "utility/log.h"

namespace ultra
{

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
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  /*const environment &get_helper(environment *) const;
  const problem &get_helper(problem *) const;
  */

  const ultra::problem *prob_;

  std::vector<layer_t> layers_ {};
};

template<Individual I>
class population<I>::layer_t
{
public:
  [[nodiscard]] std::size_t size() const;

  [[nodiscard]] bool empty() const;
  void clear();

  [[nodiscard]] std::size_t allowed() const;
  void allowed(std::size_t);

  void push_back(const I &);
  void pop_back();

  std::vector<I> members {};

private:
  std::size_t allowed_ {0};
};

#include "kernel/population.tcc"
#include "kernel/population_coord.tcc"
#include "kernel/population_iterator.tcc"

/*
template<Individual I> typename population<I>::coord pickup(const population<I> &);
template<Individual I> typename population<I>::coord pickup(
  const population<I> &, typename population<T>::coord);
*/

}  // namespace ultra

#endif  // include guard
