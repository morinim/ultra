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

#include <iterator>

#include "kernel/gp/src/holdout_validation.h"
#include "kernel/random.h"

namespace ultra::src
{

///
/// Sets up a hold-out validator.
///
/// \param[in] prob current problem
/// \param[in] training   percentage of examples used for training
/// \param[in] validation percentage of examples used for validation
///
/// Examples from `prob.data(training)` are randomly partitioned into training,
/// validation and test set according to the `training` and `validation`
/// percentages.
///
holdout_validation::holdout_validation(src::problem &prob,
                                       int training, int validation)
  : prob_(prob)
{
  auto &training_set(prob_.data(dataset_t::training));
  auto &validation_set(prob_.data(dataset_t::validation));
  auto &test_set(prob_.data(dataset_t::test));

  Expects(!training_set.empty());
  Expects(validation_set.empty());
  Expects(test_set.empty());

  const auto available(training_set.size());
  if (available <= 1)
    return;

  if (training <= 0)
  {
    training   = 60;  // %
    validation = 20;
    // test is 20 %
  }
  else if (training >= 100)
  {
    ultraWARNING << "Holdout with 100% training is unusual";
    training = 100;
  }

  if (validation < 0)
    validation = (100 - training) / 2;
  else
    validation = std::min(100 - training, validation);

  const auto n_training(std::max<std::size_t>(available * training / 100, 1));
  const auto n_validation(
    validation == 100 - training
    ? available - n_training
    : std::max<std::size_t>(available * validation / 100, 1));

  assert(n_training + n_validation <= available);

  ultraINFO << "Holdout validation settings: " << training
            << "% training (" << n_training << "), " << validation
            << "% validation (" << n_validation << "), "
            << 100 - training - validation << "% test ("
            << available - n_training - n_validation << ')';

  std::ranges::shuffle(training_set, random::engine());

  // Setup validation set.
  if (validation_set.columns.empty())
    validation_set.clone_schema(training_set);

  auto move_begin(std::next(training_set.begin(), n_training));
  auto move_end(std::next(move_begin, n_validation));
  validation_set.insert(validation_set.end(),
                        std::make_move_iterator(move_begin),
                        std::make_move_iterator(move_end));

  // Setup test set.
  if (test_set.columns.empty())
    test_set.clone_schema(training_set);

  test_set.insert(test_set.end(),
                  std::make_move_iterator(move_end),
                  std::make_move_iterator(training_set.end()));

  training_set.erase(move_begin, training_set.end());

  Expects(training_set.size() == n_training);
  Expects(validation_set.size() == n_validation);
}

void holdout_validation::training_setup(unsigned)
{
  prob_.set_data(dataset_t::training);
}

void holdout_validation::validation_setup(unsigned)
{
  prob_.set_data(dataset_t::validation);
}

std::unique_ptr<validation_strategy> holdout_validation::clone() const
{
  return std::make_unique<holdout_validation>(*this);
}

}  // namespace ultra
