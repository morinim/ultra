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
class holdout_validation : public validation_strategy
{
public:
  explicit holdout_validation(src::problem &, int = -1, int = -1);

  void training_setup(unsigned) override;
  bool shake(unsigned) override { return false; }
  void validation_setup(unsigned) override;

  [[nodiscard]] std::unique_ptr<validation_strategy> clone() const override;

private:
  src::problem &prob_;
};

}  // namespace ultra::src

#endif  // include guard
