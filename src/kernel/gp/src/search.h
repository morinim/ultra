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
#include "kernel/gp/src/problem.h"
#include "kernel/search.h"

namespace ultra::src
{

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
class basic_search : public ultra::search<ES, E>
{
public:
  using individual_t = typename ultra::search<ES, E>::individual_t;

  basic_search(problem &, E, metric_flags = metric_flags::nothing);

  std::unique_ptr<basic_oracle> oracle(const individual_t &) const;

  //basic_search &validation_strategy(validator_id);

  [[nodiscard]] bool is_valid() const override;

protected:
  // *** Template methods / customization points ***
  //void calculate_metrics(summary<T> *) const override;

  //void log_stats(const search_stats<T> &,
  //               tinyxml2::XMLDocument *) const override;

  void tune_parameters() override;

private:
  [[nodiscard]] problem &prob() const noexcept;
  //template<class E, class... Args> void set_evaluator(Args && ...);

  // *** Private data members ***
  metric_flags metrics_;  // metrics we have to calculate during the search
};  // class basic_search


template<IndividualOrTeam P = gp::individual>
class search
{
public:
  using individual_t = P;
  using fitness_t = double;

  using reg_evaluator_t = rmae_evaluator<P>;

  search(problem &p, metric_flags m = metric_flags::nothing);

  search_stats<P, fitness_t> run(unsigned = 1,
                                 const model_measurements<fitness_t> & = {});

  std::unique_ptr<basic_oracle> oracle(const P &) const;

private:
  // *** Private data members ***
  problem &prob_;  // problem we're working on
  metric_flags metrics_;  // metrics we have to calculate during the search
};


#include "kernel/gp/src/search.tcc"

}  // namespace ultra::src

#endif  // include guard
