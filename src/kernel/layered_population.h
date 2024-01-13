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

#if !defined(ULTRA_LAYERED_POPULATION_H)
#define      ULTRA_LAYERED_POPULATION_H

#include <algorithm>
#include <numeric>

#include "kernel/linear_population.h"
#include "kernel/problem.h"
#include "kernel/random.h"

#include "utility/log.h"
#include "utility/misc.h"

namespace ultra
{

///
/// A group of individuals which may interact together (for example by mating)
/// producing offspring.
///
/// Typical population size ranges from one hundred to many thousands. The
/// population is organized in one or more layers that can interact in
/// many ways (depending on the evolution strategy).
///
template<Individual I>
class layered_population
{
public:
  struct coord;
  using layer_t = linear_population<I>;
  using layer_iter = typename std::vector<layer_t>::iterator;
  using layer_const_iter = typename std::vector<layer_t>::const_iterator;

  using value_type = I;
  using difference_type = std::ptrdiff_t;

  explicit layered_population(const ultra::problem &);

  // Layer-related
  [[nodiscard]] const layer_t &front() const;
  [[nodiscard]] layer_t &front();
  [[nodiscard]] const layer_t &back() const;
  [[nodiscard]] layer_t &back();

  [[nodiscard]] std::size_t layers() const;
  [[nodiscard]] const layer_t &layer(std::size_t) const;
  [[nodiscard]] layer_t &layer(std::size_t);

  [[nodiscard]] basic_range<layer_const_iter> range_of_layers() const;
  [[nodiscard]] basic_range<layer_iter> range_of_layers();

  void init(layer_t &);
  void add_layer();
  void remove(layer_t &);

  [[nodiscard]] std::size_t size() const;

  // Misc
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

  [[nodiscard]] const value_type &operator[](const coord &c) const
  {return layers_[c.layer][c.index]; }
  [[nodiscard]] value_type &operator[](const coord &c)
  {return layers_[c.layer][c.index]; }

private:
  const ultra::problem *prob_;

  std::vector<layer_t> layers_ {};
};

template<Individual I>
[[nodiscard]] layered_population<I>
make_debug_population(const ultra::problem &);

namespace random
{

template<LayeredPopulation P>
[[nodiscard]] typename std::size_t layer(const P &);

}  // namespace random

#include "kernel/layered_population.tcc"
#include "kernel/layered_population_coord.tcc"
#include "kernel/layered_population_iterator.tcc"

}  // namespace ultra

#endif  // include guard
