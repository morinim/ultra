/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SRC_CALCULATE_METRICS_H)
#define      ULTRA_SRC_CALCULATE_METRICS_H

#include "kernel/gp/src/classification_result.h"
#include "kernel/gp/src/dataframe.h"

#include <cmath>
#include <limits>
#include <ranges>

namespace ultra::src::metrics
{

///
/// Concept for regression predictors.
///
/// A regression predictor must be callable with an input example and return a
/// `value_t`.
///
template<class P>
concept RegressionPredictor = requires(const P &p,
                                       const std::vector<value_t> &x)
{
  { p(x) } -> std::same_as<value_t>;
};

///
/// Concept for classification predictors.
///
/// A classification predictor must expose `tag()` returning a
/// `classification_result`.
///
template<class P>
concept ClassificationPredictor = requires(const P &p,
                                           const std::vector<value_t> &x)
{
  { p.tag(x) } -> std::same_as<classification_result>;
};

///
/// Accuracy for classification problems.
///
/// \param[in] p oracle-like object
/// \param[in] d classification dataset
/// \return      fraction of correctly classified examples
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
template<ClassificationPredictor P>
[[nodiscard]] double accuracy(const P &p, const dataframe &d)
{
  Expects(d.classes() >= 2);
  Expects(d.size());

  const auto ok(std::ranges::count_if(
                  d,
                  [&p](const auto &e)
                  {
                    return p.tag(e.input).label == label(e);
                  }));

  return static_cast<double>(ok) / static_cast<double>(d.size());
}

///
/// Mean absolute error for regression problems.
///
/// Missing predictions are ignored. If no valid prediction is produced, the
/// function returns `infinity()`.
///
/// \param[in] p oracle-like object
/// \param[in] d regression dataset
/// \return      mean absolute error over valid predictions
///
template<RegressionPredictor P>
[[nodiscard]] double mae(const P &p, const dataframe &d)
{
  Expects(!d.classes());
  Expects(d.size());

  double sum(0.0);
  std::size_t n(0);

  for (const auto &e : d)
    if (const auto res(p(e.input)); has_value(res))
    {
      sum += std::fabs(std::get<D_DOUBLE>(res) - label_as<D_DOUBLE>(e));
      ++n;
    }

  return n ? sum / static_cast<double>(n)
           : std::numeric_limits<double>::infinity();
}

}  // namespace ultra::src::metrics

#endif  // include guard
