/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_COLUMNS_INFO_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_COLUMNS_INFO_TCC)
#define      ULTRA_COLUMNS_INFO_TCC

namespace internal
{

// Move the `n`-th element to the front, preserving the order of the other
// elements.
// If `n` is empty add an empty column to the front.
template<std::ranges::random_access_range R>
[[nodiscard]] R output_column_first(const R &raw, std::optional<std::size_t> n)
{
  R r(raw);

  if (n)
  {
    assert(n < r.size());

    if (n > 0)
      std::rotate(r.begin(),
                  std::next(r.begin(), *n),
                  std::next(r.begin(), *n + 1));
  }
  else
    // When the output index is missing, all the columns are treated as input
    // columns (this is obtained adding a surrogate, empty output column).
    r.insert(r.begin(), "");

  return r;
}

}  // namespace internal

///
/// Given an example compiles information about the columns of the dataframe.
///
/// \param[in] exs a range containing examples. The first row must contain
///                column headers
/// \param[in] output_index index of the column to be considered as output
///
template<RangeOfSizedRanges R>
void columns_info::build(R exs, std::optional<std::size_t> output_index)
{
  Expects(exs.size());
  Expects(exs.front().size());

  std::ranges::transform(
    exs, exs.begin(),
    [&output_index](const auto &r)
    {
      return internal::output_column_first(r, output_index);
    });

  // Set up column headers (first row must contain the headers).
  cols_.reserve(exs.front().size());
  std::ranges::transform(exs.front(), std::back_inserter(cols_),
                         [this](const auto &name)
                         {
                           return column_info(*this, trim(name));
                         });

  for (std::size_t idx(0); idx < size(); ++idx)
    for (auto row(std::next(exs.begin())); row != exs.end(); ++row)
    {
      if (row->size() <= idx)
        continue;

      const auto &value(*std::next(row->begin(), idx));

      // Sets the domain associated to a column.
      switch (cols_[idx].domain())
      {
      case d_void:
        if (is_integer(value))
          cols_[idx].domain(d_int);
        else if (is_number(value))
          cols_[idx].domain(d_double);
        else if (value != "")
          cols_[idx].domain(d_string);
        break;

      case d_int:
        if (is_integer(value))
          continue;

        cols_[idx].domain(d_double);
        [[fallthrough]];

      case d_double:
        if (is_number(value))
          continue;
        [[fallthrough]];

      default:
        if (value != "")
          cols_[idx].domain(d_string);
      }
    }

  // For classification tasks we use discriminant functions and the actual
  // output type is always numeric.
  if (cols_.front().domain() == d_string)
    cols_.front().domain(d_double);
}

#endif  // include guard
