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
#include "kernel/search_log.h"
#include "kernel/search_stats.h"
#include "kernel/validation_strategy.h"

namespace ultra
{

///
/// basic_search drives the evolution.
///
/// The class offers a general / customizable search strategy.
///
template<template<class> class ES, Evaluator E>
class basic_search
{
public:
  using individual_t = evaluator_individual_t<E>;
  using fitness_t = evaluator_fitness_t<E>;
  using after_generation_callback_t =
    ultra::after_generation_callback_t<individual_t, fitness_t>;

  basic_search(problem &, E);

  search_stats<individual_t, fitness_t> run(
    unsigned = 1, const model_measurements<fitness_t> & = {});

  template<ValidationStrategy, class... Args>
  basic_search &validation_strategy(Args && ...);
  basic_search &validation_strategy(const ultra::validation_strategy &);

  basic_search &after_generation(after_generation_callback_t);

  [[nodiscard]] virtual bool is_valid() const;

protected:
  // ---- Template methods ----
  virtual void after_evolution(
    unsigned, const std::vector<model_measurements<fitness_t>> &);
  [[nodiscard]] virtual model_measurements<fitness_t> calculate_metrics(
    const individual_t &) const;
  virtual void tune_parameters();

  // ---- Data members ----
  ES<evaluator_proxy<E>> es_;

  std::unique_ptr<ultra::validation_strategy> vs_
    {std::make_unique<as_is_validation>()};

  problem &prob_;  // problem we're working on

  // Callback functions.
  after_generation_callback_t after_generation_callback_ {};

private:
  // Template method of the `basic_search::run` member function called exactly
  // one time just before the first run.
  virtual void init();

  bool load();
  //bool save() const;

  mutable search_log search_log_ {};
};  // class basic_search


template<Evaluator E>
class search : public basic_search<alps_es, E>
{
public:
  search(problem &, E);
};

#include "kernel/search.tcc"
}  // namespace ultra

#endif  // include guard
