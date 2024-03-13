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

#include <map>

#include "kernel/distribution.h"
#include "kernel/evaluator.h"
#include "kernel/population.h"

namespace ultra
{

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

  void add(const I &, const F &, unsigned = 0);

  void clear();

  [[nodiscard]] const distribution<double> &age_dist() const;
  [[nodiscard]] const distribution<F> &fit_dist() const;
  [[nodiscard]] const distribution<double> &length_dist() const;

  template<Population P> [[nodiscard]] const distribution<double> &age_dist(
    const P &) const;
  template<Population P> [[nodiscard]] const distribution<F> &fit_dist(
    const P &) const;
  template<Population P> [[nodiscard]] const distribution<double> &length_dist(
    const P &) const;

  [[nodiscard]] bool is_valid() const;

private:
  struct group_stat
  {
    distribution<double>    age {};
    distribution<F>     fitness {};
    distribution<double> length {};
  };
  std::map<unsigned, group_stat> group_stat_ {};

  distribution<F>         fit_ {};
  distribution<double>    age_ {};
  distribution<double> length_ {};
};  // analyzer

#include "kernel/analyzer.tcc"

}  // namespace ultra

#endif  // include guard
