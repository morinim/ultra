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

#if !defined(ULTRA_HOLDOUT_VALIDATION_H)
#define      ULTRA_HOLDOUT_VALIDATION_H

#include "kernel/validation_strategy.h"
#include "kernel/gp/src/problem.h"

namespace ultra::src
{

///
/// Holdout validation, aka *one round cross-validation* or *conventional
/// validation*.
///
/// Holdout validation involves partitioning a sample of data into
/// complementary subsets, performing the analysis on one subset (called the
/// training set) and validating the analysis on the other subset (called the
/// validation set).
///
/// \see
/// https://en.wikipedia.org/wiki/Training,_validation,_and_test_data_sets
///
class holdout_validation : public validation_strategy
{
public:
  struct params
  {
    params() noexcept {}

    /// Percentage of the dataset used for training.
    ///
    /// - Valid range: `1...100`;
    /// - values `<= 0` default to `70`;
    /// - values `> 100` are clamped to `100`.
    int training_perc {70};

    /// Percentage of the dataset used for validation.
    ///
    /// - Valid range: `0...(100 - training_perc)`;
    /// - if negative, it is set to `100 - training_perc`;
    /// - if too large, it is clamped to `100 - training_perc`.
    int validation_perc {30};

    /// Whether or not to shuffle the data before splitting.
    bool shuffle {true};

    /// Some classification problems can exhibit a large imbalance in the
    /// distribution of the target classes: for instance there could be several
    /// times more negative samples than positive samples. In such cases it's
    /// recommended to use stratified sampling to ensure that relative class
    /// frequencies is approximately preserved in train and validation sets.
    /// \remark
    /// `stratify` is ignored for symbolic regression problems.
    bool stratify {true};
  };

  explicit holdout_validation(src::problem &, params = {});

  void training_setup(unsigned) override;

  // Holdout validation uses a single, fixed split
  bool shake(unsigned) override { return false; }

  bool validation_setup(unsigned) override;

  [[nodiscard]] std::unique_ptr<validation_strategy> clone() const override;

private:
  src::problem &prob_;
};

}  // namespace ultra::src

#endif  // include guard
