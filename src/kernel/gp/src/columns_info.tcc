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

///
/// Normalises a row by moving the designated output column to the front.
///
/// \param[in] raw input row
/// \param[in] n   optional index of the output column
/// \returns       a range whose first element represents the output column.
///                The returned range may be a lazy view or an owning
///                container, depending on library support.
///
/// \pre If `n` is provided, it must be a valid index into `raw`
///
/// If `n` is provided, the element at position `n` is moved to index `0`,
/// preserving the relative order of the other elements in the prefix
/// `[0, n]`. Elements with index greater than `n` are left unchanged.
///
/// If `n` is empty, a surrogate empty element is inserted at the front,
/// treating all original elements as input columns.
///
template<DataframeRow R>
[[nodiscard]] auto output_column_first(const R &raw,
                                       std::optional<std::size_t> n)
{
  using VT = std::ranges::range_value_t<R>;

#if defined(__cpp_lib_ranges_concat)  // lazy view
  if (n)
  {
    assert(*n < static_cast<std::size_t>(std::ranges::distance(raw)));

    return raw
           | std::views::drop(*n) | std::views::take(1)
           | std::views::concat(raw | std::views::take(*n))
           | std::views::concat(raw | std::views::drop(*n + 1));
  }

  // When the output index is missing, all the columns are treated as input
  // columns (this is obtained adding a surrogate, empty output column).

  // Empty surrogate column + original row.
  return std::views::single(VT()) | std::views::concat(raw);
#else  // eager container
  R r(raw);

  if (n)
  {
    assert(*n < static_cast<std::size_t>(std::ranges::distance(raw)));

    // `ranges::rotate` has better efficiency on common implementations with
    // `bidirectional_iterator` or (better) `random_access_iterator`.
    // Implementations (e.g. MSVC STL) may enable vectorization when the
    // iterator type models `contiguous_iterator` and swapping its value type
    // calls neither non-trivial special member function nor ADL-found swap.
    if (*n > 0)
      std::rotate(r.begin(),
                  std::next(r.begin(), *n),
                  std::next(r.begin(), *n + 1));
  }
  else
  {
    // When the output index is missing, all the columns are treated as input
    // columns (this is obtained adding a surrogate, empty output column).
    if constexpr (std::same_as<VT, std::string>)
      r.insert(r.begin(), "");
    else
      r.insert(r.begin(), D_VOID());
  }

  return r;
#endif
}

}  // namespace internal

///
/// Compiles metadata describing dataframe columns from a sample of rows.
///
/// \param[in] exs          a non-empty range of rows. Each row must itself be
///                         a sized range. The first row must contain column
///                         headers.
/// \param[in] output_index optional index of the output column
///
/// \pre `exs` must not be empty, and the header row must contain at least
///            one column
/// The first row of `exs` is interpreted as the header row. An optional output
/// column may be designated via `output_index`; when present, the
/// corresponding column is treated as the output and is normalised to appear
/// first during analysis.
///
/// To limit computational cost, domain inference is performed on a bounded
/// prefix of the input rows.
///
/// \note
/// Rows with insufficient length for a given column index are ignored for that
/// column during domain inference.
///
template<DataframeMatrix R>
void columns_info::build(const R &exs, std::optional<std::size_t> output_index)
{
  using VT = std::ranges::range_value_t<std::ranges::range_value_t<R>>;

  Expects(!exs.empty());
  Expects(exs.front().size());

  cols_.clear();

  // Lazily reorders each row so that the output column is first. Also limits
  // the analysis to a subset of the available rows.
  constexpr std::size_t max_domain_samples {1000};
  const auto normalised_rows(
    exs | std::views::take(max_domain_samples)
    | std::views::transform([&output_index](const auto &r)
      {
        return internal::output_column_first(r, output_index);
      }));

  // Set up column headers (first row must contain the headers).
  const auto &header_row(*normalised_rows.begin());

  cols_.reserve(std::ranges::distance(header_row));

  std::ranges::transform(
    header_row, std::back_inserter(cols_),
    [this](const auto &name)
    {
      if constexpr (std::same_as<VT, std::string>)
        return column_info(*this, trim(name));
      else
        return column_info(*this, trim(lexical_cast<std::string>(name)));
    });

  // Domain inference.
  for (std::size_t idx(0); idx < size(); ++idx)
    for (const auto &row : normalised_rows | std::views::drop(1))
    {
      if (row.size() <= idx)
        continue;

      const auto &value(*std::next(row.begin(), idx));

      // Sets the domain associated to a column.
      switch (cols_[idx].domain())
      {
      case d_void:
        if constexpr (std::same_as<VT, std::string>)
        {
          if (is_integer(value))
            cols_[idx].domain(d_int);
          else if (is_number(value))
            cols_[idx].domain(d_double);
          else if (value != "")
            cols_[idx].domain(d_string);
        }
        else
        {
          if (basic_data_type(value))
            cols_[idx].domain(static_cast<domain_t>(value.index()));
        }
        break;

      case d_int:
        if constexpr (std::same_as<VT, std::string>)
        {
          if (is_integer(value))
            continue;
        }
        else
        {
          if (value.index() == d_int)
            continue;
        }

        cols_[idx].domain(d_double);
        [[fallthrough]];

      case d_double:
        if constexpr (std::same_as<VT, std::string>)
        {
          if (is_number(value))
            continue;
        }
        else
        {
          if (numerical_data_type(value))
            continue;
        }
        [[fallthrough]];

      default:
        if constexpr (std::same_as<VT, std::string>)
        {
          if (!value.empty())
            cols_[idx].domain(d_string);
        }
        else
        {
          if (value.index() == d_string)
            cols_[idx].domain(d_string);
        }
      }
    }

  settle_task_t();
}

#endif  // include guard
