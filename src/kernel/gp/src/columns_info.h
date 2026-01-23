/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_COLUMNS_INFO_H)
#define      ULTRA_COLUMNS_INFO_H

#include "kernel/symbol.h"
#include "kernel/value.h"
#include "utility/misc.h"

#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ultra::src
{

///
/// Strategy used for data typing.
///
/// - `weak`. Types are inferred permissively: columns **that share the same
///   domain** (e.g. `double` with `double`, `string` with `string`...) can be
///   freely mixed by the engine;
/// - `strong`. Types are inferred strictly: every column has its own
///   type/category (Strongly Typed Genetic Programming).
///
/// Even when specifying `typing::weak` the engine won't mix all the columns.
/// In particular, a unique category will be assigned to:
/// - columns associated with distinct domains;
/// - columns with `d_string` domain.
///
/// \see
/// https://github.com/morinim/ultra/wiki/bibliography#10
///
enum class typing {weak, strong};

///
/// Type of learning task associated with the dataframe.
///
/// - `classification`: discrete output variable;
/// - `regression`: continuous output variable;
/// - `unsupervised`: no designated output variable.
///
enum class task_t {classification, regression, unsupervised};

///
/// A single row of a generic dataframe.
///
/// A DataframeRow models one logical record of a dataframe. It must:
/// - be a `forward_range` (multi-pass iteration is required);
/// - be a `sized_range` (the number of columns must be known);
/// - be copy constructible (rows may be copied during inference);
/// - expose values of type `value_t` or `std::string`.
///
/// This concept deliberately does not require random access, allowing
/// rows to be represented by lightweight views or containers.
///
template<class R>
concept DataframeRow =
  std::ranges::forward_range<R>
  && std::ranges::sized_range<R>
  && std::copy_constructible<R>
  && (std::same_as<std::ranges::range_value_t<R>, value_t>
      || std::same_as<std::ranges::range_value_t<R>, std::string>);

///
/// A collection of dataframe rows.
///
/// A `DataframeMatrix` represents a full tabular dataset. It must:
/// - be a forward_range and sized_range;
/// - have elements that satisfy DataframeRow.
///
/// The outer range corresponds to rows, while the inner range corresponds to
/// columns within each row.
///
template<class R> concept DataframeMatrix =
  std::ranges::forward_range<R>
  && std::ranges::sized_range<R>
  && DataframeRow<std::ranges::range_value_t<R>>;

///
/// Metadata container describing the columns of a dataframe.
///
/// This class stores and manages column-level information such as:
/// - column names;
/// - inferred domains;
/// - input/output roles;
/// - typing constraints.
///
/// It also provides facilities to infer such metadata from a dataframe-like
/// range and to query the resulting schema.
///
/// \related dataframe
///
class columns_info
{
public:
  /// Information about a single column of the dataset.
  class column_info
  {
  public:
    explicit column_info(const columns_info &,
                         const std::string & = "",
                         domain_t = d_void,
                         const std::set<value_t> & = {});

    [[nodiscard]] const std::string &name() const noexcept;
    void name(const std::string &);

    [[nodiscard]] domain_t domain() const noexcept;
    void domain(domain_t);

    [[nodiscard]] const std::set<value_t> &states() const noexcept;
    void add_state(value_t);

    [[nodiscard]] symbol::category_t category() const;

  private:
    const columns_info *owner_ {nullptr};

    std::string         name_;
    domain_t          domain_;
    std::set<value_t> states_;
  };

  // ---- Aliases ----
  using const_iterator = std::vector<column_info>::const_iterator;
  using iterator = std::vector<column_info>::iterator;
  using size_type = std::size_t;
  using value_type = column_info;

  // ---- Constructors ----
  columns_info() = default;
  columns_info(const columns_info &);
  columns_info(const std::vector<std::pair<std::string, domain_t>> &);

  columns_info &operator=(const columns_info &);

  // ---- Element access ----
  [[nodiscard]] const column_info &operator[](size_type) const;
  [[nodiscard]] column_info &operator[](size_type);

  [[nodiscard]] const column_info &operator[](const std::string &) const;

  [[nodiscard]] const column_info &front() const;
  [[nodiscard]] column_info &front();

  [[nodiscard]] const column_info &back() const;
  [[nodiscard]] column_info &back();

  // ---- Capacity ----
  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  // ---- Iterators ----
  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  // ---- Modifiers ----
  void pop_back();
  void push_back(const column_info &);
  void push_front(const column_info &);

  // ---- Misc ----
  void data_typing(typing) noexcept;

  template<DataframeMatrix R>
  void build(const R &, std::optional<std::size_t>);

  std::set<symbol::category_t> used_categories() const;

  [[nodiscard]] domain_t domain_of_category(symbol::category_t) const;

  [[nodiscard]] task_t task() const noexcept;

  [[nodiscard]] bool is_valid() const;

private:
  [[nodiscard]] symbol::category_t category(const column_info &) const;
  [[nodiscard]] domain_t evaluation_domain(const column_info &) const;
  void settle_task_t();

  std::vector<column_info> cols_ {};
  typing typing_ {typing::weak};

  task_t task_ {task_t::regression};
};

#include "columns_info.tcc"

}  // namespace ultra::src

#endif  // include guard
