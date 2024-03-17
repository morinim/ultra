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

#if !defined(ULTRA_SEARCH_H)
#define      ULTRA_SEARCH_H

#include "kernel/evaluator_proxy.h"
#include "kernel/evolution.h"
#include "kernel/model_measurements.h"
#include "kernel/problem.h"
#include "kernel/validation_strategy.h"

namespace ultra
{

template<Individual I, Fitness F>
struct search_stats
{
  void update(const I &, const model_measurements<F> &);

  I best_individual {};
  model_measurements<F> measurements {};

  distribution<F> fd {};
  std::set<unsigned> good_runs {};

  unsigned best_run {0};  /// index of the run giving the best solution
  unsigned runs     {0};  /// number of runs performed so far
};

///
/// Search drives the evolution.
///
/// The class offers a general / customizable search strategy.
///
template<template<Evaluator> class ES, Evaluator E>
class search
{
public:
  using individual_t = evaluator_individual_t<E>;
  using fitness_t = evaluator_fitness_t<E>;

  search(problem &, E);

  search_stats<individual_t, fitness_t> run(unsigned = 1);

  template<class V, class... Args> search &validation_strategy(Args && ...);

  [[nodiscard]] virtual bool is_valid() const;

protected:
  virtual void tune_parameters();

  // *** Data members ***
  ES<evaluator_proxy<E>> es_;
  std::unique_ptr<ultra::validation_strategy> vs_;

  problem &prob_;    // problem we're working on

private:
  // Template method of the `search::run` member function called exactly one
  // time just before the first run.
  virtual void init();

  // Template method of the `search::run` member function called exactly one
  // time after every evolution run..
  [[nodiscard]] virtual model_measurements<fitness_t> calculate_metrics(
    const individual_t &) const;

  //void log_stats(const search_stats<T> &) const;
  bool load();
  //bool save() const;
};  // class search


template<Evaluator E>
class alps_search : public search<alps_es, E>
{
public:
  alps_search(problem &, E);
};

#include "kernel/search.tcc"
}  // namespace ultra

#endif  // include guard
