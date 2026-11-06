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

#if !defined(ULTRA_STANDARD_SCALER_H)
#define      ULTRA_STANDARD_SCALER_H

#include "kernel/gp/src/dataframe.h"

#include <string>
#include <utility>
#include <vector>

namespace ultra::src
{

///
/// Standardises numerical columns of a dataframe to zero mean and unit variance.
///
class standard_scaler
{
public:
  struct column_stats
  {
    double anchor {0.0};
    double scale {1.0};
    double mean {0.0};      ///< Mean offset from anchor, in scale units
    double std_dev {1.0};   ///< Standard deviation in scale units
  };

  struct options
  {
    bool scale_inputs {true};   ///< Standardise feature columns
    bool scale_output {false};  ///< Standardise target column (opt-in for MAE/MSE)
  };

  standard_scaler() = default;
  explicit standard_scaler(options) noexcept;

  void fit(const dataframe &);
  void transform(dataframe &) const;

  [[nodiscard]] double inverse_transform_target(double) const;

  [[nodiscard]] bool is_fitted() const noexcept { return is_fitted_; }
  [[nodiscard]] bool is_valid() const noexcept
  {
    return !is_fitted_
           || (!schema_.empty() && input_stats_.size() + 1 == schema_.size());
  }

private:
  std::vector<std::pair<std::string, domain_t>> schema_ {};
  std::vector<column_stats> input_stats_ {};
  column_stats output_stats_ {};

  options opt_ {};
  bool is_fitted_ {false};
};

}  // namespace ultra::src

#endif  // include guard
