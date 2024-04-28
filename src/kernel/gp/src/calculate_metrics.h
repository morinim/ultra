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

#if !defined(ULTRA_SRC_CALCULATE_METRICS_H)
#define      ULTRA_SRC_CALCULATE_METRICS_H

#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/oracle.h"

namespace ultra::src
{

class core_reg_oracle;

///
/// There are a lot of metrics related to a model / `oracle` and we don't want
/// fat classes.
/// The `Visitor` pattern is ideal to simplify the interface of `oracle` and
/// keep possibility for future expansions (new metrics).
///
/// This works quite well since metrics can be implemented in terms of the
/// public interface of `basic_oracle`.
///
class model_metric
{
public:
  [[nodiscard]] virtual double operator()(const core_reg_oracle *,
                                          const dataframe &) const = 0;
  //[[nodiscard]] virtual double operator()(const core_class_oracle *,
  //                                        const dataframe &) const = 0;
};

///
/// Accuracy refers to the number of training examples that are correctly
/// valued/classified as a proportion of the total number of examples in
/// the training set.
///
/// According to this design, the best accuracy is `1.0` (100%), meaning that
/// all the training examples have been correctly recognized.
///
/// \note
/// **Accuracy and fitness aren't the same thing**.
/// Accuracy can be used to measure fitness but often it hasn't enough
/// "granularity"; also it isn't appropriated for classification tasks with
/// imbalanced learning data (where at least one class is under/over
/// represented relative to others).
///
class accuracy_metric : public model_metric
{
public:
  [[nodiscard]] double operator()(const core_reg_oracle *,
                                  const dataframe &) const override;

  //[[nodiscard]] double operator()(const core_class_oracle *,
  //                                const dataframe &) const override;
};

}  // namespace ultra::src

#endif  // include guard
