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

#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "kernel/value.h"

namespace ultra::src
{

/// A category provide operations which supplement or supersede those of the
/// domain but which are restricted to values lying in the (sub)domain by
/// which is parametrized.
/// For instance the constant `4.0` (in the domain `d_double`) may belong to
/// two distinct categories: `2 =="km/h"` and `3 == "kg"`).
/// Categories are the way strong typing GP is enforced in Ultra.
using category_t = std::size_t;
constexpr category_t undefined_category = static_cast<category_t>(-1);

///
/// Category/type management of the dataframe columns.
///
/// - `weak`: columns having the same domain can be freely mixed by the engine.
/// - `strong`: every column has its own type/category (Strongly Typed Genetic
/// Programming).
///
/// Even when specifying `typing::weak` the engine won't mix all the columns.
/// Particularly a unique category will be assigned to:
/// - columns associated with distinct domains;
/// - columns with `d_string` domain.
///
/// \see
/// https://github.com/morinim/ultra/wiki/bibliography#10
///
enum class typing {weak, strong};

///
/// Information about the collection of columns (type, name, output index).
///
/// \related
/// dataframe
///
class columns_info
{
public:
  /// Information about a single column of the dataset.
  class column_info
  {
  public:
    explicit column_info(const columns_info &, const std::string & = "");

    std::string         name {};
    domain_t          domain {d_void};
    std::set<value_t> states {};

    [[nodiscard]] category_t category() const;

  private:
    const columns_info *owner_ {nullptr};
  };

  using const_iterator = std::vector<column_info>::const_iterator;
  using iterator = std::vector<column_info>::iterator;
  using size_type = std::size_t;
  using value_type = column_info;

  columns_info() = default;

  void data_typing(typing) noexcept;

  [[nodiscard]] const column_info &operator[](size_type) const;
  [[nodiscard]] column_info &operator[](size_type);

  [[nodiscard]] size_type size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] const_iterator begin() const noexcept;
  [[nodiscard]] iterator begin() noexcept;
  [[nodiscard]] const_iterator end() const noexcept;

  [[nodiscard]] const column_info &front() const;
  [[nodiscard]] column_info &front();

  [[nodiscard]] const column_info &back() const;
  [[nodiscard]] column_info &back();

  void pop_back();
  void push_back(const column_info &);
  void push_front(const column_info &);

  void build(const std::vector<std::string> &, bool);
  std::set<category_t> used_categories() const;

  [[nodiscard]] bool is_valid() const;

private:
  [[nodiscard]] category_t category(const column_info &) const;

  std::vector<column_info> cols_ {};
  typing typing_ {typing::weak};
};

}  // namespace ultra::src

#endif  // include guard
