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
  struct layer;

  explicit population(const ultra::problem &);

  void init_layer(std::size_t);
  [[nodiscard]] std::size_t allowed(std::size_t) const;
  [[nodiscard]] std::size_t individuals(std::size_t) const;
  [[nodiscard]] std::size_t layers() const;
  void pop_from_layer(std::size_t);

  [[nodiscard]] I &operator[](const coord &);
  [[nodiscard]] const I &operator[](const coord &) const;

  [[nodiscard]] std::size_t individuals() const;

  [[nodiscard]] const ultra::problem &problem() const;

  // Iterators.
  template<bool> class base_iterator;
  using const_iterator = base_iterator<true>;
  using iterator = base_iterator<false>;

  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] bool is_valid() const;

/*
  void add_layer();
  void add_to_layer(unsigned, const T &);
  void remove_layer(unsigned);
  void set_allowed(unsigned, unsigned);

  void inc_age();

  const problem &get_problem() const;

  // Serialization.
  bool load(std::istream &, const problem &);
  bool save(std::ostream &) const;
*/
private:
  /*const environment &get_helper(environment *) const;
  const problem &get_helper(problem *) const;
  */

  const ultra::problem *prob_;

  std::vector<layer> layers_;
};

template<Individual I>
struct population<I>::layer
{
  std::vector<I> members {};
  std::size_t    allowed {0};
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
