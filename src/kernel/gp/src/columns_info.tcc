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

[[nodiscard]] constexpr std::size_t normalised_column_count(
    std::size_t raw_count, std::optional<std::size_t> output_index) noexcept
{
  return raw_count + (output_index ? 0 : 1);
}

[[nodiscard]] constexpr std::optional<std::size_t> raw_column_index(
  std::size_t idx, std::optional<std::size_t> output_index) noexcept
{
  if (!output_index)
    return idx == 0 ? std::nullopt : std::optional<std::size_t>(idx - 1);

  if (idx == 0)
    return *output_index;

  return idx <= *output_index ? idx - 1 : idx;
}

template<DataframeRow R>
class normalised_row_view
{
public:
  using value_type = std::ranges::range_value_t<R>;

  normalised_row_view(const R &r, std::optional<std::size_t> output_index)
    : r_(&r), output_index_(output_index),
      logical_size_(normalised_column_count(std::ranges::distance(r),
                                            output_index))
  {
    Expects(!output_index_ || *output_index_ < std::ranges::distance(r));
  }

  [[nodiscard]] std::size_t size() const noexcept { return logical_size_; }

  class iterator
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = normalised_row_view::value_type;
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;

    iterator() = default;

    iterator(const normalised_row_view *view, std::size_t pos)
      : view_(view), pos_(pos)
    {
      refresh_raw_index();

      if (raw_idx_)
      {
        it_ = std::ranges::begin(*view_->r_);
        std::advance(it_, *raw_idx_);
      }
    }

    [[nodiscard]] const value_type &operator*() const
    {
      if (!raw_idx_)
        return view_->empty_;

      return *it_;
    }

    iterator &operator++()
    {
      const auto previous_raw_idx(raw_idx_);

      ++pos_;

      if (pos_ >= view_->logical_size_)
      {
        raw_idx_.reset();
        return *this;
      }

      refresh_raw_index();

      if (!raw_idx_)
        return *this;

      if (previous_raw_idx && *raw_idx_ == *previous_raw_idx + 1)
        ++it_;
      else
      {
        it_ = std::ranges::begin(*view_->r_);
        std::advance(it_, *raw_idx_);
      }

      return *this;
    }

    iterator operator++(int)
    {
      auto old(*this);
      ++*this;
      return old;
    }

    [[nodiscard]] bool operator==(const iterator &other) const noexcept
    {
      return view_ == other.view_ && pos_ == other.pos_;
    }

  private:
    void refresh_raw_index()
    {
      raw_idx_ = pos_ < view_->logical_size_
                 ? view_->raw_column(pos_)
                 : std::nullopt;
    }

    const normalised_row_view *view_ {};
    std::size_t pos_ {};
    std::optional<std::size_t> raw_idx_ {};
    std::ranges::iterator_t<const R> it_ {};
  };

  [[nodiscard]] iterator begin() const { return iterator(this, 0); }
  [[nodiscard]] iterator end() const { return iterator(this, logical_size_); }

private:
  [[nodiscard]] std::optional<std::size_t> raw_column(
    std::size_t pos) const noexcept
  {
    return raw_column_index(pos, output_index_);
  }

  const R *r_;
  std::optional<std::size_t> output_index_;
  std::size_t logical_size_;
  const value_type empty_ {};
};

}  // namespace internal

///
/// Infer column metadata from a dataframe-like range.
///
/// \tparam R a `DataframeMatrix` type
///
/// \param[in] exs          a non-empty range of rows representing the dataset.
///                         The first row must contain column headers
/// \param[in] output_index optional index of the output column. If not
///                         provided, a surrogate output column may be inserted
///
/// \pre The range must not be empty
/// \pre Rows must have non-zero size.
///
/// The first row of `exs` is interpreted as the header row. An optional output
/// column may be designated via `output_index`; when present, the
/// corresponding column is treated as the output and is normalised to appear
/// first during analysis.
///
/// To limit computational cost, domain inference is performed on a bounded
/// prefix of the input rows.
///
/// \remark
/// Rows whose normalised width differs from the header are ignored during
/// domain inference.
///
template<DataframeMatrix R>
void columns_info::build(const R &exs, std::optional<std::size_t> output_index)
{
  using VT = std::ranges::range_value_t<std::ranges::range_value_t<R>>;

  Expects(!std::ranges::empty(exs));
  Expects(std::ranges::size(*std::ranges::begin(exs)));

  constexpr std::size_t max_domain_samples(1000);

  // Set up column headers (first row must contain the headers).
  const internal::normalised_row_view header_row(*std::ranges::begin(exs),
                                                 output_index);

  cols_.clear();
  cols_.reserve(header_row.size());

  for (const auto &name : header_row)
    if constexpr (std::same_as<VT, std::string>)
      cols_.emplace_back(*this, trim(name));
    else
      cols_.emplace_back(*this, trim(lexical_cast<std::string>(name)));

  const auto update_domain([this](std::size_t idx, const auto &value)
  {
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
          return;
      }
      else
      {
        if (value.index() == d_int)
          return;
      }

      cols_[idx].domain(d_double);
      [[fallthrough]];

    case d_double:
      if constexpr (std::same_as<VT, std::string>)
      {
        if (is_number(value))
          return;
      }
      else
      {
        if (numerical_data_type(value))
          return;
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
  });

  // Domain inference.
  for (const auto sample_rows(exs | std::views::take(max_domain_samples)
                                  | std::views::drop(1));
       const auto &row : sample_rows)
  {
    if (output_index
        && *output_index >=
           static_cast<std::size_t>(std::ranges::distance(row)))
      continue;

    if (const internal::normalised_row_view normalised(row, output_index);
        normalised.size() == size())
      for (std::size_t idx(0); const auto &value : normalised)
      {
        update_domain(idx, value);
        ++idx;
      }
  }

  settle_task_t();
}

#endif  // include guard
