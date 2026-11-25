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

#if !defined(ULTRA_DATAFRAME_SUMMARY_H)
#define      ULTRA_DATAFRAME_SUMMARY_H

#include "kernel/distribution.h"
#include "kernel/gp/src/dataframe.h"

namespace ultra::src
{

///
/// Summary statistics for an empty or unassigned (void) column.
///
struct void_stats {};

///
/// Summary statistics for a numerical column.
///
struct numeric_stats
{
  distribution<double> dist {};  ///< Distribution of observed values

  [[nodiscard]] std::pair<double, double> range() const noexcept
  { return {dist.min(), dist.max()}; }
};

///
/// Summary statistics for a categorical column.
///
struct categorical_stats
{
  std::size_t count {0};       ///< Total number of observations
  std::size_t unique {0};      ///< Number of distinct categories
  std::string mode {};         ///< The most frequently occurring category
  std::size_t mode_count {0};  ///< The frequency of the mode
};

/// Variant of statistical summaries for a column.
using column_stats_t = std::variant<void_stats, numeric_stats,
                                    categorical_stats>;

///
/// Summary metadata and statistics for a single dataframe column.
///
struct column_description
{
  std::string name {};       ///< Column name / header
  domain_t domain {d_void};  ///< Domain of the column
  column_stats_t stats {};   ///< Statistical summary (numeric or categorical)
};

///
/// Summary of descriptive statistics for a dataframe.
///
/// Computes summary statistics across all columns of a dataframe,
/// producing distributions (mean, std dev, min, max) for numerical
/// columns and frequency tables (unique count, mode, mode frequency)
/// for categorical columns.
///
/// \related dataframe
///
class dataframe_summary
{
public:
  explicit dataframe_summary(const dataframe &);

  /// \return number of rows in the summarized dataframe
  [[nodiscard]] std::size_t rows() const noexcept { return rows_; }
  /// \return number of columns in the summarized dataframe
  [[nodiscard]] std::size_t columns() const noexcept { return cols_.size(); }

  [[nodiscard]] const column_description &operator[](std::size_t) const;
  [[nodiscard]] const column_description &operator[](std::string_view) const;

  [[nodiscard]] std::optional<std::pair<double, double>> range(std::size_t) const;
  [[nodiscard]] std::optional<std::pair<double, double>> range(
    std::string_view) const;

  friend std::ostream &operator<<(std::ostream &, const dataframe_summary &);

private:
  std::vector<column_description> cols_ {};
  std::size_t rows_ {0};
};  // class dataframe_summary

[[nodiscard]] inline dataframe_summary describe(const dataframe &d)
{
  return dataframe_summary(d);
}

}  // namespace ultra::src

#endif  // include guard
