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

#include "kernel/gp/src/standard_scaler.h"
#include "kernel/exceptions.h"

#include <algorithm>
#include <cmath>

namespace ultra::src
{

///
/// Constructs a standard scaler.
///
/// \param[in] opt scaler options
///
standard_scaler::standard_scaler(options opt) noexcept : opt_(opt)
{
}

///
/// Computes mean and standard deviation from a reference dataset.
///
/// \param[in] df reference dataframe
///
void standard_scaler::fit(const dataframe &df)
{
  Expects(!df.empty());

  const auto vars(df.variables());

  schema_.clear();
  schema_.reserve(df.columns.size());
  for (const auto &column : df.columns)
    schema_.emplace_back(column.name(), column.domain());

  input_stats_.assign(vars, {});
  output_stats_ = {};

  std::vector<std::size_t> in_counts(vars, 0);
  std::vector<std::pair<double, double>> in_ranges(vars);
  std::size_t out_count(0);
  std::pair<double, double> out_range {};

  // Find each numeric column's range before choosing its scale.
  for (const auto &ex : df)
  {
    if (opt_.scale_inputs)
      for (unsigned i(0); i < vars; ++i)
        if (const auto *val{std::get_if<D_DOUBLE>(&ex.input[i])})
        {
          if (in_counts[i] == 0)
            in_ranges[i] = {*val, *val};
          else
          {
            in_ranges[i].first = std::min(in_ranges[i].first, *val);
            in_ranges[i].second = std::max(in_ranges[i].second, *val);
          }
          ++in_counts[i];
        }

    if (opt_.scale_output)
      if (const auto *val{std::get_if<D_DOUBLE>(&ex.output)})
      {
        if (out_count == 0)
          out_range = {*val, *val};
        else
        {
          out_range.first = std::min(out_range.first, *val);
          out_range.second = std::max(out_range.second, *val);
        }
        ++out_count;
      }
  }

  for (unsigned i(0); i < vars; ++i)
    if (in_counts[i] > 1 && in_ranges[i].first < in_ranges[i].second)
    {
      input_stats_[i].anchor = in_ranges[i].first;
      input_stats_[i].scale = std::max(std::abs(in_ranges[i].first),
                                     std::abs(in_ranges[i].second));
    }
    else if (in_counts[i] > 0)
      input_stats_[i].anchor = in_ranges[i].first;

  if (out_count > 1 && out_range.first < out_range.second)
  {
    output_stats_.anchor = out_range.first;
    output_stats_.scale = std::max(std::abs(out_range.first),
                                  std::abs(out_range.second));
  }
  else if (out_count > 0)
    output_stats_.anchor = out_range.first;

  // Accumulate centred, scaled variance without squaring original values.
  std::vector<std::size_t> in_seen(vars, 0);
  std::size_t out_seen(0);
  std::vector<double> in_m2(vars, 0.0);
  double out_m2(0.0);

  for (const auto &ex : df)
  {
    if (opt_.scale_inputs)
      for (unsigned i(0); i < vars; ++i)
        if (const auto *val{std::get_if<D_DOUBLE>(&ex.input[i])})
        {
          auto &stats(input_stats_[i]);
          if (in_ranges[i].first < in_ranges[i].second)
          {
            const double offset(*val / stats.scale - stats.anchor / stats.scale);
            const double delta(offset - stats.mean);
            stats.mean += delta / static_cast<double>(++in_seen[i]);
            in_m2[i] += delta * (offset - stats.mean);
          }
        }

    if (opt_.scale_output)
      if (const auto *val{std::get_if<D_DOUBLE>(&ex.output)})
      {
        if (out_range.first < out_range.second)
        {
          const double offset(*val / output_stats_.scale
                              - output_stats_.anchor / output_stats_.scale);
          const double delta(offset - output_stats_.mean);
          output_stats_.mean += delta / static_cast<double>(++out_seen);
          out_m2 += delta * (offset - output_stats_.mean);
        }
      }
  }

  for (unsigned i(0); i < vars; ++i)
    if (in_seen[i] > 1)
      input_stats_[i].std_dev = std::sqrt(in_m2[i]
                                          / static_cast<double>(in_seen[i] - 1));

  if (out_seen > 1)
    output_stats_.std_dev = std::sqrt(out_m2
                                      / static_cast<double>(out_seen - 1));

  is_fitted_ = true;
  Ensures(is_valid());
}

///
/// Applies standardisation in-place to a dataframe.
///
/// \param[in,out] df dataframe to transform
///
void standard_scaler::transform(dataframe &df) const
{
  Expects(is_fitted());

  if (!df.empty())
  {
    if (df.columns.size() != schema_.size())
      throw exception::data_format("Dataframe schema differs from fitted scaler");

    for (std::size_t i(0); i < schema_.size(); ++i)
      if (df.columns[i].name() != schema_[i].first
          || df.columns[i].domain() != schema_[i].second)
        throw exception::data_format("Dataframe schema differs from fitted scaler");
  }

  for (auto &ex : df)
  {
    if (opt_.scale_inputs)
      for (unsigned i(0); i < input_stats_.size(); ++i)
        if (double *val = std::get_if<D_DOUBLE>(&ex.input[i]))
        {
          const auto &stats(input_stats_[i]);
          *val = (*val / stats.scale - stats.anchor / stats.scale - stats.mean)
                 / stats.std_dev;
        }

    if (opt_.scale_output)
      if (double *val = std::get_if<D_DOUBLE>(&ex.output))
        *val = (*val / output_stats_.scale
                - output_stats_.anchor / output_stats_.scale
                - output_stats_.mean) / output_stats_.std_dev;
  }
}

///
/// Inverts a standardised target value to its original scale.
///
/// \param[in] norm_y standardised target value
/// \return           value in original units
///
double standard_scaler::inverse_transform_target(double norm_y) const
{
  Expects(is_fitted());

  if (!opt_.scale_output)
    return norm_y;

  return std::fma(norm_y * output_stats_.std_dev + output_stats_.mean,
                  output_stats_.scale, output_stats_.anchor);
}

}  // namespace ultra::src
