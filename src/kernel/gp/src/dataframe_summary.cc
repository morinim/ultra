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

#include "kernel/gp/src/dataframe_summary.h"
#include "utility/assert.h"
#include "utility/misc.h"

#include <algorithm>
#include <format>
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace ultra::src
{

///
/// Computes descriptive statistics for a dataframe.
///
/// \param[in] d dataframe to summarize
///
dataframe_summary::dataframe_summary(const dataframe &d)
  : cols_(d.columns.size()), rows_(d.size())
{
  std::vector<std::unordered_map<std::string, unsigned>> frequencies(
    cols_.size());

  for (std::size_t i(0); i < d.columns.size(); ++i)
  {
    cols_[i].name = d.columns[i].name();
    cols_[i].domain = d.columns[i].domain();

    if (cols_[i].domain == d_string)
      cols_[i].stats = categorical_stats();
    else if (cols_[i].domain != d_void)
      cols_[i].stats = numeric_stats();
  }

  const auto update_column([&](const value_t &val, std::size_t i)
  {
    if (auto *num = std::get_if<numeric_stats>(&cols_[i].stats))
      num->dist.add(lexical_cast<double>(val));
    else if (auto *cat = std::get_if<categorical_stats>(&cols_[i].stats))
    {
      ++cat->count;

      const std::string sval(trim(lexical_cast<std::string>(val)));
      const auto count(++frequencies[i][sval]);

      if (count == 1)
        ++cat->unique;

      if (count > cat->mode_count)
      {
        cat->mode = sval;
        cat->mode_count = count;
      }
    }
  });

  const bool has_output(!cols_.empty() && cols_[0].domain != d_void);

  for (const auto &row : d)
  {
    if (has_output)
      update_column(row.output, 0);

    for (std::size_t i(0); i < row.input.size(); ++i)
      update_column(row.input[i], i + 1);
  }
}

///
/// Accesses column description by index.
///
/// \param[in] i column index
/// \return      constant reference to the column description
///
const column_description &dataframe_summary::operator[](std::size_t i) const
{
  Expects(i < cols_.size());
  return cols_[i];
}

///
/// Accesses column description by column name.
///
/// \param[in] name column name to search for
/// \return         constant reference to the column description
///
/// \exception `std::out_of_range` if no column with the specified name exists
///
const column_description &dataframe_summary::operator[](
  std::string_view name) const
{
  const auto it(std::ranges::find(cols_, name, &column_description::name));

  if (it == cols_.end())
    throw std::out_of_range("Column name not found");

  return *it;
}

///
/// Minimum and maximum range for a numeric column.
///
/// \param[in] i column index
/// \return      a pair containing {min, max} if the column is numeric and
///              non-empty, or `std::nullopt` otherwise
///
std::optional<std::pair<double, double>> dataframe_summary::range(
  std::size_t i) const
{
  Expects(i < cols_.size());

  if (const auto *num = std::get_if<numeric_stats>(&cols_[i].stats))
    if (!num->dist.empty())
      return num->range();

  return std::nullopt;
}

///
/// Minimum and maximum range for a numeric column.
///
/// \param[in] name column name to search for
/// \return         a pair containing {min, max} if the column is found,
///                 numeric, and non-empty, or `std::nullopt` otherwise
///
std::optional<std::pair<double, double>> dataframe_summary::range(
  std::string_view name) const
{
  const auto it(std::ranges::find(cols_, name, &column_description::name));
  return it != cols_.end()
         ? range(static_cast<std::size_t>(std::distance(cols_.begin(), it)))
         : std::nullopt;
}

///
/// Stream insertion operator for dataframe summary.
///
/// \param[in,out] os output stream
/// \param[in]     s  dataframe summary to format
/// \return           reference to the updated output stream
///
/// Pretty-prints a tabular summary in pandas-like style.
///
/// \related dataframe_summary
///
std::ostream &operator<<(std::ostream &os, const dataframe_summary &s)
{
  if (s.cols_.empty())
    return os << "[empty dataframe_summary]\n";

  const bool has_numeric(
    std::ranges::any_of(
      s.cols_,
      [](const auto &c)
      {
        return std::holds_alternative<numeric_stats>(c.stats);
      }));

  const bool has_categorical(
    std::ranges::any_of(
      s.cols_,
      [](const auto &c)
      {
        return std::holds_alternative<categorical_stats>(c.stats);
      }));

  std::vector<std::string_view> metrics;
  if (has_numeric && !has_categorical)
    metrics = {"count", "mean", "std", "min", "max"};
  else if (!has_numeric && has_categorical)
    metrics = {"count", "unique", "top", "freq"};
  else
    metrics = {"count", "unique", "top", "freq", "mean", "std", "min", "max"};

  const auto col_name([](const column_description &c, std::size_t j)
  {
    return c.name.empty() ? std::format("[{}]", j) : c.name;
  });

  const auto cell_value([](const column_description &c,
                           std::string_view metric) -> std::string
  {
    if (std::holds_alternative<void_stats>(c.stats))
      return "N/A";

    if (const auto *num = std::get_if<numeric_stats>(&c.stats))
    {
      if (metric == "count")
        return std::to_string(num->dist.size());
      if (num->dist.empty())
        return "NaN";
      if (metric == "mean")
        return std::format("{:.6g}", num->dist.mean());
      if (metric == "std")
        return num->dist.size() > 1
               ? std::format("{:.6g}", num->dist.standard_deviation())
               : "NaN";
      if (metric == "min")
        return std::format("{:.6g}", num->dist.min());
      if (metric == "max")
        return std::format("{:.6g}", num->dist.max());
      return "NaN";
    }

    if (const auto *cat = std::get_if<categorical_stats>(&c.stats))
    {
      if (metric == "count")
        return std::to_string(cat->count);
      if (metric == "unique")
        return std::to_string(cat->unique);
      if (metric == "top")
        return cat->mode.empty() ? "NaN" : cat->mode;
      if (metric == "freq")
        return cat->mode_count ? std::to_string(cat->mode_count) : "NaN";
      return "NaN";
    }

    return "NaN";
  });

  std::size_t metric_width(8);
  for (const auto &m : metrics)
    metric_width = std::max(metric_width, m.size());

  std::vector<std::size_t> col_widths(s.cols_.size());
  for (std::size_t j(0); j < s.cols_.size(); ++j)
  {
    col_widths[j] = std::max<std::size_t>(8, col_name(s.cols_[j], j).size());
    for (const auto &m : metrics)
      col_widths[j] = std::max(col_widths[j], cell_value(s.cols_[j], m).size());
  }

  const auto flags(os.flags());

  // Header row.
  os << std::string(metric_width, ' ');
  for (std::size_t j(0); j < s.cols_.size(); ++j)
    os << "  " << std::setw(col_widths[j]) << col_name(s.cols_[j], j);
  os << '\n';

  // Metric rows.
  for (const auto &m : metrics)
  {
    os << std::left << std::setw(metric_width) << m << std::right;
    for (std::size_t j(0); j < s.cols_.size(); ++j)
      os << "  " << std::setw(col_widths[j]) << cell_value(s.cols_[j], m);
    os << '\n';
  }

  os.flags(flags);
  return os;
}

}  // namespace ultra::src
