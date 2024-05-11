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

}  // namespace

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

  if (training_set.size() <= 1)
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

  assert(training + validation <= 100);

  if (validation_set.columns.empty())
    validation_set.clone_schema(training_set);
  if (test_set.columns.empty())
    test_set.clone_schema(training_set);

  std::vector<example> input_set;
  input_set.insert(input_set.end(),
                   std::make_move_iterator(training_set.begin()),
                   std::make_move_iterator(training_set.end()));
  training_set.clear();

  std::ranges::shuffle(input_set, random::engine());

  const std::vector strata(
    prob.classification()
    ? std::vector{basic_range(input_set.begin(), input_set.end())}
    : stratification(input_set));

  for (auto &stratum : strata)
  {
    const auto available(std::ranges::size(stratum));
    assert(available);

    const auto n_training(std::max<std::size_t>(available * training / 100, 1));
    assert(n_training);

    std::size_t n_validation(0);
    if (const auto remaining(available - n_training); remaining)
    {
      if (validation == 100 - training)
        n_validation = remaining;
      else
        n_validation = std::max<std::size_t>(available * validation / 100, 1);
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

  ultraINFO << "Holdout validation settings: " << training
            << "% training (" << training_set.size() << "), " << validation
            << "% validation (" << validation_set.size() << "), "
            << 100 - training - validation << "% test ("
            << test_set.size() << ')';
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
