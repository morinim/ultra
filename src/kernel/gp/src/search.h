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

#if !defined(ULTRA_SRC_SEARCH_H)
#define      ULTRA_SRC_SEARCH_H

#include "kernel/gp/src/evaluator.h"
#include "kernel/gp/src/metrics.h"
#include "kernel/gp/src/problem.h"
#include "kernel/search.h"

#include <variant>

namespace ultra::src
{

template<Individual P = gp::individual> class search;

enum class metric_flags : unsigned
{
  nothing = 0x0000,

  accuracy = 1 << 0,
  f1_score = 1 << 1,

  everything = 0xFFFF
};

///
/// Drives the search for solutions of symbolic regression / classification
/// tasks.
///
template<template<class> class ES, Evaluator E>
class basic_search : public ultra::basic_search<ES, E>
{
public:
  using individual_t = typename ultra::basic_search<ES, E>::individual_t;
  using fitness_t = typename ultra::basic_search<ES, E>::fitness_t;

  explicit basic_search(problem &, metric_flags = metric_flags::accuracy);

  [[nodiscard]] auto oracle(const individual_t &) const;

  [[nodiscard]] bool is_valid() const override;

protected:
  // ---- Template methods / customization points ----
  [[nodiscard]] model_measurements<fitness_t> calculate_metrics(
    const individual_t &) const override;

  void tune_parameters() override;

private:
  template<Individual> friend class search;

  [[nodiscard]] problem &prob() const noexcept;

  // ---- Private data members ----
  metric_flags metrics_;  // metrics we have to calculate during the search
};  // class basic_search

///
/// Thin facade over the concrete classification or regression search engine.
///
/// The engine kind is selected at construction time from the problem type.
/// Configuration calls are forwarded directly to the selected engine.
///
template<Individual P>
class search
{
public:
  // ---- Member types ----
  using individual_t = P;
  using fitness_t = double;

  using after_generation_callback_t =
    ultra::after_generation_callback_t<P, fitness_t>;
  using on_training_new_best_callback_t =
    ultra::on_new_best_callback_t<P, fitness_t>;

  using class_evaluator_t = gaussian_evaluator<P>;
  using reg_evaluator_t = rmae_evaluator<P>;

  // ---- Constructor ----
  explicit search(problem &, metric_flags = metric_flags::accuracy);

  // ---- Run ----
  search_stats<P, fitness_t> run(unsigned = 1,
                                 const model_measurements<fitness_t> & = {});

  [[nodiscard]] std::unique_ptr<basic_oracle> oracle(const P &) const;

  // ---- Hooks ----
  search &after_generation(after_generation_callback_t);
  search &logger(search_log &);
  search &messages(bool) noexcept;
  search &on_training_new_best(on_training_new_best_callback_t);
  template<class F> search &refinement(F &&);
  search &stop_source(std::stop_source);
  search &tag(const std::string &);

  template<ValidationStrategy, class... Args>
  search &validation_strategy(Args && ...);

private:
  using class_search_t = basic_search<alps_es, class_evaluator_t>;
  using reg_search_t = basic_search<alps_es, reg_evaluator_t>;
  using engine_t = std::variant<class_search_t, reg_search_t>;

  [[nodiscard]] static engine_t make_engine(problem &, metric_flags);
  [[nodiscard]] bool problem_type_unchanged() const noexcept;

  // ---- Private data members ----
  engine_t engine_;
  const problem &prob_;
  const bool classification_;  // problem type captured at construction time
};


#include "kernel/gp/src/search.tcc"

}  // namespace ultra::src

#endif  // include guard
