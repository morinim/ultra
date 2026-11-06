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
        if (const auto *val = std::get_if<D_DOUBLE>(&ex.input[i]))
        {
          if (in_counts[i])
          {
            in_ranges[i].first = std::min(in_ranges[i].first, *val);
            in_ranges[i].second = std::max(in_ranges[i].second, *val);
          }
          else
            in_ranges[i] = {*val, *val};

          ++in_counts[i];
        }

    if (opt_.scale_output)
      if (const auto *val = std::get_if<D_DOUBLE>(&ex.output))
      {
        if (out_count)
        {
          out_range.first = std::min(out_range.first, *val);
          out_range.second = std::max(out_range.second, *val);
        }
        else
          out_range = {*val, *val};

        ++out_count;
      }
  }

  const auto init_anchor_scale([](column_stats &stats, std::size_t count,
                                  std::pair<double, double> range)
  {
    if (count)
    {
      stats.anchor = range.first;
      if (range.first < range.second)  // this also implies `count > 1`
        stats.scale = std::max(std::abs(range.first), std::abs(range.second));
    }
  });

  for (unsigned i(0); i < vars; ++i)
    init_anchor_scale(input_stats_[i], in_counts[i], in_ranges[i]);

  init_anchor_scale(output_stats_, out_count, out_range);

  // Accumulate centred, scaled variance without squaring original values.
  std::vector<std::size_t> in_seen(vars, 0);
  std::size_t out_seen(0);
  std::vector<double> in_m2(vars, 0.0);
  double out_m2(0.0);

  const auto update_welford([](double val, column_stats &stats,
                               std::size_t &seen, double &m2)
  {
    const double offset(val / stats.scale - stats.anchor / stats.scale);
    const double delta(offset - stats.mean);
    stats.mean += delta / static_cast<double>(++seen);
    m2 += delta * (offset - stats.mean);
  });

  for (const auto &ex : df)
  {
    if (opt_.scale_inputs)
      for (unsigned i(0); i < vars; ++i)
        if (const auto *val = std::get_if<D_DOUBLE>(&ex.input[i]))
          if (in_ranges[i].first < in_ranges[i].second)
            update_welford(*val, input_stats_[i], in_seen[i], in_m2[i]);

    if (opt_.scale_output)
      if (const auto *val = std::get_if<D_DOUBLE>(&ex.output))
        if (out_range.first < out_range.second)
          update_welford(*val, output_stats_, out_seen, out_m2);
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
    const bool valid(
      (df.columns.size() == schema_.size())
      && std::ranges::equal(df.columns, schema_,
                            [](const auto &col, const auto &s)
                            {
                              return col.name() == s.first
                                     && col.domain() == s.second;
                            }));

    if (!valid)
      throw exception::data_format(
        "Dataframe schema differs from fitted scaler");
  }

  const auto standardize([](double val, const column_stats &stats)
  {
    return (val / stats.scale - stats.anchor / stats.scale - stats.mean)
           / stats.std_dev;
  });

  for (auto &ex : df)
  {
    if (opt_.scale_inputs)
      for (unsigned i(0); i < input_stats_.size(); ++i)
        if (double *val = std::get_if<D_DOUBLE>(&ex.input[i]))
          *val = standardize(*val, input_stats_[i]);

    if (opt_.scale_output)
      if (double *val = std::get_if<D_DOUBLE>(&ex.output))
        *val = standardize(*val, output_stats_);
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

///
/// \return `true` if the object passes the internal consistency check
///
bool standard_scaler::is_valid() const noexcept
{
  return !is_fitted_
         || (!schema_.empty() && input_stats_.size() + 1 == schema_.size());
}

}  // namespace ultra::src
