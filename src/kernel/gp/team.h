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

#if !defined(ULTRA_TEAM_H)
#define      ULTRA_TEAM_H

#include <algorithm>

#include "kernel/cache.h"
#include "kernel/individual.h"

namespace ultra
{
///
/// A collection of cooperating individuals used as a member of
/// ultra::population.
///
/// In generals teams of individuals can be implemented in different ways.
/// * Firstly, a certain number of individuals can be selected randomly from
///   the population and evaluated in combination as a team (but we have a
///   credit assignment problem).
/// * Secondly, team members can be evolved in separate subpopulations which
///   provide a more specialized development.
/// * We can use an explicit team representation that is considered as one
///   individual by the evolutionary algorithm. The population is subdivided
///   into fixed, equal-sized groups of individuals. Each program is
///   assigned a fixed position index in its team (program vector). The
///   members of a team undergo a coevolutionary process because they are
///   always selected, evaluated and varied simultaneously. This eliminates
///   the credit assignment problem and renders the composition of teams an
///   object of evolution.
///
/// \note
/// The team size has to be large enough to cause an improved prediction
/// compared to the traditional approach, i.e. team size one (but the
/// complexity of the search space and the training time, respectively,
/// grow exponentially with the number of coevolved programs).
///
/// \see https://github.com/morinim/ultra/wiki/bibliography#16
///
template<Individual I>
class team
{
public:
  team() = default;
  explicit team(std::size_t);
  explicit team(const problem &);
  explicit team(std::vector<I>);

  // Recombination operators.
  unsigned mutation(const problem &);

  [[nodiscard]] const I &operator[](std::size_t) const;

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] ultra::individual::age_t age() const;
  void inc_age(unsigned = 1);

  [[nodiscard]] bool is_valid() const;

  // Iterators.
  using members_t = std::vector<I>;
  using const_iterator = typename members_t::const_iterator;
  using value_type = typename members_t::value_type;
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  // Serialization.
  bool load(std::istream &, const symbol_set &);
  bool save(std::ostream &) const;

  template<Individual U> friend team<U> crossover(const team<U> &,
                                                  const team<U> &);

private:
  // Private support methods.
  [[nodiscard]] hash_t hash() const;

  // Private data members.
  members_t individuals_ {};

  mutable hash_t signature_ {};
};

// ***********************************************************************
// *  Comparison operators                                               *
// ***********************************************************************
template<Individual I> [[nodiscard]] bool operator==(const team<I> &,
                                                     const team<I> &);
template<Individual I> [[nodiscard]] bool operator!=(const team<I> &,
                                                     const team<I> &);
template<Individual I> [[nodiscard]] unsigned distance(const team<I> &,
                                                       const team<I> &);


template<Individual I> [[nodiscard]] unsigned active_slots(const team<I> &);
template<Individual I> [[nodiscard]] team<I> crossover(const team<I> &,
                                                       const team<I> &);

template<Individual I> std::ostream &operator<<(std::ostream &,
                                                const team<I> &);


template<class T> concept Team = requires(T t)
{
  // See https://stackoverflow.com/q/71921797/3235496
  // This C++20 template lambda only binds to `team<I>` specialisations,
  // including classes derived from them.
  //
  []<Individual I>(team<I> &){}(t);
};

#include "kernel/gp/team.tcc"

}  // namespace ultra

#endif  // include guard
