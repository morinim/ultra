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

#if !defined(ULTRA_ANALYZER_H)
#define      ULTRA_ANALYZER_H

#include <concepts>
#include <future>
#include <vector>

#include "kernel/distribution.h"
#include "kernel/evaluator.h"
#include "kernel/population.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"

namespace ultra
{

template<Individual I, Fitness F>
struct group_stat
{
  explicit group_stat(population_uid);

  void add(const I &, const F &);
  void merge(group_stat);

  population_uid uid;

  distribution<double>    age {};
  distribution<F>     fitness {};
  distribution<double> length {};
};

///
/// Analyzer takes a statistics snapshot of a population.
///
/// Procedure:
/// 1. the population set should be loaded adding one individual at time
///    (analyzer::add method);
/// 2. statistics can be checked calling specific methods.
///
/// You can get information about:
/// - the set as a whole (`age_dist()`, `fit_dist()`, `length_dist()`);
/// - grouped information (`age_dist(unsigned)`, `fit_dist(unsigned)`...).
///
template<Individual I, Fitness F>
class analyzer
{
public:
  analyzer() = default;
  template<LayeredPopulation P, Evaluator E> explicit analyzer(
    const P &, const E &);

  void add(const I &, const F &, population_uid);

  void clear();

  [[nodiscard]] group_stat<I, F> overall_group_stat() const;

  [[nodiscard]] distribution<double> age_dist() const;
  [[nodiscard]] distribution<F> fit_dist() const;
  [[nodiscard]] distribution<double> length_dist() const;

  [[nodiscard]] const distribution<double> &age_dist(population_uid) const;
  [[nodiscard]] const distribution<F> &fit_dist(population_uid) const;
  [[nodiscard]] const distribution<double> &length_dist(population_uid) const;

  template<Population P> [[nodiscard]] const distribution<double> &age_dist(
    const P &) const;
  template<Population P> [[nodiscard]] const distribution<F> &fit_dist(
    const P &) const;
  template<Population P> [[nodiscard]] const distribution<double> &length_dist(
    const P &) const;

  [[nodiscard]] bool is_valid() const;

private:
  [[nodiscard]] group_stat<I, F> *group(population_uid);
  [[nodiscard]] const group_stat<I, F> *group(population_uid) const;

  std::vector<group_stat<I, F>> group_stat_ {};
};  // analyzer

template<LayeredPopulation P, Evaluator E>
analyzer(const P &, const E &) -> analyzer<typename P::value_type,
                                           evaluator_fitness_t<E>>;

#include "kernel/analyzer.tcc"

}  // namespace ultra

#endif  // include guard
