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

template<class C, class T>
concept has_insert_at_beginning = requires(C c, T v)
{
  { c.begin() } -> std::input_iterator; // has a begin() returning an iterator
  { c.insert(c.begin(), v) };           // supports insert at begin
};

template<class C>
concept range_with_insert_at_beginning =
  std::ranges::range<C>
  && has_insert_at_beginning<C, std::ranges::range_value_t<C>>;

// Move the `n`-th element to the front, preserving the order of the other
// elements.
// If `n` is empty add an empty column to the front.
//
// \note
// `ranges::rotate` has better efficiency on common implementations with
// `bidirectional_iterator` or (better) `random_access_iterator`.
// Implementations (e.g. MSVC STL) may enable vectorization when the iterator
// type models `contiguous_iterator` and swapping its value type calls neither
// non-trivial special member function nor ADL-found swap.
template<range_with_insert_at_beginning R>
[[nodiscard]] R output_column_first(const R &raw, std::optional<std::size_t> n)
{
  R r(raw);

  if (n)
  {
    assert(n < std::ranges::distance(raw));

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
/// Compiles metadata about the dataframe columns from a sample of examples.
///
/// \param[in] exs          a range of examples. The first row must contain
///                         column headers
/// \param[in] output_index optional index specifying the output column
///
/// \note
/// Examples with an incorrect number of columns are skipped.
///
template<RangeOfSizedRanges R>
void columns_info::build(R exs, std::optional<std::size_t> output_index)
{
  using VT = std::ranges::range_value_t<std::ranges::range_value_t<R>>;
  static_assert(std::same_as<VT, value_t> || std::same_as<VT, std::string>);

  Expects(std::ranges::distance(exs));
  Expects(exs.front().size());

  std::ranges::transform(
    exs, exs.begin(),
    [&output_index](const auto &r)
    {
      return internal::output_column_first(r, output_index);
    });

  // Set up column headers (first row must contain the headers).
  cols_.reserve(std::ranges::distance(exs.front()));
  std::ranges::transform(
    exs.front(), std::back_inserter(cols_),
    [this](const auto &name)
    {
      if constexpr (std::same_as<VT, std::string>)
        return column_info(*this, trim(name));
      else
        return column_info(*this, trim(std::get<std::string>(name)));
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
            cols_[idx].domain(value.index());
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

  // For classification tasks we use discriminant functions and the actual
  // output type is always numeric.
  if (cols_.front().domain() == d_string)
    cols_.front().domain(d_double);
}

#endif  // include guard
