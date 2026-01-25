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

#include "kernel/gp/src/holdout_validation.h"
#include "kernel/random.h"

#include <iterator>

namespace
{

template<std::ranges::range R>
[[nodiscard]] std::vector<ultra::basic_range<std::ranges::iterator_t<R>>>
stratification(R &container)
{
  std::vector<ultra::basic_range<std::ranges::iterator_t<R>>> ret;

  for (auto begin_it(container.begin()); begin_it != container.end(); )
  {
    const auto current_class(begin_it->output);

    const auto end_it(std::partition(begin_it, container.end(),
                                     [current_class](const auto &example)
                                     {
                                       return example.output == current_class;
                                     }));

    ret.push_back({begin_it, end_it});

    begin_it = end_it;
  };

  return ret;
}

template<std::ranges::range R>
void split_dataset(R &stratum, int training_perc, int validation_perc,
                   ultra::src::dataframe &training_set,
                   ultra::src::dataframe &validation_set,
                   ultra::src::dataframe &test_set)
{
  const auto available(std::ranges::size(stratum));
  assert(available);

  const auto n_training(std::max<std::size_t>(available * training_perc / 100,
                                              1));
  assert(n_training);

  std::size_t n_validation(0);
  if (const auto remaining(available - n_training); remaining)
  {
    if (validation_perc == 100 - training_perc)
      n_validation = remaining;
    else
      n_validation = std::max<std::size_t>(available * validation_perc / 100,
                                           1);
  }

  assert(n_training + n_validation <= available);

  const auto move_end1(std::next(stratum.begin(), n_training));
  training_set.insert(training_set.end(),
                      std::make_move_iterator(stratum.begin()),
                      std::make_move_iterator(move_end1));

  const auto move_end2(std::next(move_end1, n_validation));
  validation_set.insert(validation_set.end(),
                        std::make_move_iterator(move_end1),
                        std::make_move_iterator(move_end2));

  test_set.insert(test_set.end(),
                  std::make_move_iterator(move_end2),
                  std::make_move_iterator(stratum.end()));
}

}  // namespace

namespace ultra::src
{

///
/// Sets up a hold-out validator.
///
/// \param[in] prob current problem
/// \param[in] par  prarameters for controlling the splitting / subsampling
///
/// Examples from `prob.data(training)` are randomly partitioned into training,
/// validation and test set according to parameters contained in `par`.
///
holdout_validation::holdout_validation(src::problem &prob, params par)
  : prob_(prob)
{
  auto &training_set(prob_.data[dataset_t::training]);
  auto &validation_set(prob_.data[dataset_t::validation]);
  auto &test_set(prob_.data[dataset_t::test]);

  Expects(!training_set.empty());
  Expects(validation_set.empty());
  Expects(test_set.empty());

  if (training_set.size() <= 1)
    return;

  if (par.training_perc <= 0)
    par.training_perc = 70;  // %
  else if (par.training_perc >= 100)
  {
    ultraWARNING << "Holdout with 100% training is unusual";
    par.training_perc = 100;
  }

  if (const int remaining_perc(100 - par.training_perc);
      par.validation_perc < 0)
    par.validation_perc = remaining_perc;
  else if (par.validation_perc > remaining_perc)
    par.validation_perc = remaining_perc;

  // test percentage is implicitly: 100 - training - validation
  assert(par.training_perc + par.validation_perc <= 100);

  validation_set.clone_schema(training_set);
  test_set.clone_schema(training_set);

  dataframe input_set;
  input_set.clone_schema(training_set);
  input_set.swap(training_set);

  if (par.shuffle)
    std::ranges::shuffle(input_set, random::engine());

  const std::vector strata(
    prob.classification() && par.stratify
    ? stratification(input_set)
    : std::vector{basic_range(input_set.begin(), input_set.end())});

  for (auto &stratum : strata)
    split_dataset(stratum, par.training_perc, par.validation_perc,
                  training_set, validation_set, test_set);

  ultraINFO << "Holdout validation settings: " << par.training_perc
            << "% training (" << training_set.size() << "), "
            << par.validation_perc << "% validation ("
            << validation_set.size() << "), "
            << 100 - par.training_perc - par.validation_perc << "% test ("
            << test_set.size() << ')';
}

void holdout_validation::training_setup(unsigned)
{
  prob_.data.select(dataset_t::training);
}

bool holdout_validation::validation_setup(unsigned)
{
  prob_.data.select(dataset_t::validation);
  return true;
}

std::unique_ptr<validation_strategy> holdout_validation::clone() const
{
  return std::make_unique<holdout_validation>(*this);
}

}  // namespace ultra
