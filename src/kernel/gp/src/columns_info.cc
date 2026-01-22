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

#include "kernel/gp/src/columns_info.h"
#include "utility/assert.h"
#include "utility/misc.h"

#include <algorithm>

namespace ultra::src
{

///
/// \param[in] csi    owner `columns_info` object
/// \param[in] name   name of the column
/// \param[in] domain domain of the column (must be a basic data type)
/// \param[in] states possible restriction to a set of values
///
columns_info::column_info::column_info(const columns_info &csi,
                                       const std::string &name,
                                       domain_t domain,
                                       const std::set<value_t> &states)
  : owner_(&csi), name_(name), domain_(domain), states_(states)
{
  Expects(basic_data_type(domain));
}

const std::string &columns_info::column_info::name() const noexcept
{
  return name_;
}

void columns_info::column_info::name(const std::string &n)
{
  name_ = n;
}

domain_t columns_info::column_info::domain() const noexcept
{
  return domain_;
}

void columns_info::column_info::domain(domain_t d)
{
  Expects(basic_data_type(d));
  domain_ = d;
}

const std::set<value_t> &columns_info::column_info::states() const noexcept
{
  return states_;
}

void columns_info::column_info::add_state(value_t s)
{
  Expects(basic_data_type(s));
  Expects(s.index() == domain());
  states_.insert(s);
}

///
/// Computes and returns the category assigned to this column.
///
/// \note
/// This is a computed property: if the value needs to be used multiple times,
/// consider storing it in a local variable.
///
symbol::category_t columns_info::column_info::category() const
{
  return owner_ ? owner_->category(*this) : symbol::undefined_category;
}

///
/// Copy constructor.
///
/// \param[in] other another container to be used as source to initialize the
///            elements of the container with
///
/// Constructs the container with the copy of the contents of `other`.
///
columns_info::columns_info(const columns_info &other)
{
  operator=(other);
}

///
/// Constructs the object from a user provided-schema.
///
/// \param[in] schema user-provided schema for the dataframe
///
columns_info::columns_info(
  const std::vector<std::pair<std::string, domain_t>> &schema)
{
  cols_.reserve(schema.size());

  for (const auto &[name, domain] : schema)
    cols_.push_back(column_info(*this, trim(name), domain));

  settle_task_t();
}

task_t columns_info::task() const noexcept
{
  return task_;
}

void columns_info::settle_task_t()
{
  switch (cols_.front().domain())
  {
  case d_string:
    // For classification tasks we use discriminant functions and the actual
    // output type is always numeric.
    cols_.front().domain(d_int);
    task_ = task_t::classification;
    break;

  case d_void:
    task_ = task_t::unsupervised;
    break;

  default:
    task_ = task_t::regression;
  }
}

///
/// Copy assignment operator.
///
/// \param[in] other another container to use as the data source
/// \return          `*this`
///
/// Replaces the contents with a copy of the contents of other.
///
columns_info &columns_info::operator=(const columns_info &other)
{
  // A user defined copy assignment operator (and copy constructor) is required
  // because the default copy operator fails to correctly initialize
  // `column_info::owner_`.
  if (this != &other)
  {
    cols_.clear();
    for (auto &c : other.cols_)
      cols_.emplace_back(*this, c.name(), c.domain(), c.states());

    typing_ = other.typing_;
    task_ = other.task_;
  }

  return *this;
}

const columns_info::column_info &columns_info::operator[](size_type i) const
{
  return cols_[i];
}

columns_info::column_info &columns_info::operator[](size_type i)
{
  return cols_[i];
}

///
/// Returns a reference to column_info with specified column header.
///
/// \param[in] name the name of the column_info to find
/// \return         a reference to the column_info with the given column header
///
/// \exception `std::out_of_range` if the container does not contain a column
///                                with the specified name
///
const columns_info::column_info &columns_info::operator[](
  const std::string &name) const
{
  const auto it(std::ranges::find_if(
                  cols_,
                  [name](const auto &c) { return c.name() == name; }));

  if (it == cols_.end())
    throw std::out_of_range("Column name doesn't exist");

  return *it;
}

///
/// Set the data typing system used for category identification.
///
/// \param[in] t type of data typing (see `typing` for details)
///
void columns_info::data_typing(typing t) noexcept
{
  typing_ = t;
}

///
/// \return the number of columns in the container
///
columns_info::size_type columns_info::size() const noexcept
{
  return cols_.size();
}

bool columns_info::empty() const noexcept
{
  return cols_.empty();
}

columns_info::const_iterator columns_info::begin() const noexcept
{
  return cols_.begin();
}

columns_info::iterator columns_info::begin() noexcept
{
  return cols_.begin();
}

columns_info::const_iterator columns_info::end() const noexcept
{
  return cols_.end();
}

///
/// Returns a reference to the first element in the container.
///
/// \warning
/// Calling front on an empty container causes undefined behavior.
///
const columns_info::column_info &columns_info::front() const
{
  return cols_.front();
}

columns_info::column_info &columns_info::front()
{
  return cols_.front();
}

///
/// Returns a reference to the last element in the container.
///
/// \warning
/// Calling `back` on an empty container causes undefined behavior.
///
const columns_info::column_info &columns_info::back() const
{
  return cols_.back();
}

columns_info::column_info &columns_info::back()
{
  return cols_.back();
}

///
/// Removes the last column from the container.
///
/// \warning
/// Ensure the container is not empty before calling (possible undefined
/// behavior).
///
void columns_info::pop_back()
{
  cols_.pop_back();
}

///
/// Adds a new column at the end of the column list.
///
/// \param[in] v information about the new column
///
void columns_info::push_back(const column_info &v)
{
  cols_.push_back(v);
}

///
/// Inserts a new column at the beginning of the list, moving existing
/// columns forward.
///
/// \param[in] v information about the new column
///
void columns_info::push_front(const column_info &v)
{
  cols_.insert(begin(), v);
  settle_task_t();
}

///
/// \param[in] ci a column
/// \return       the domain of the column as seen by the evaluation engine
///               (not the stored domain)
///
domain_t columns_info::evaluation_domain(const column_info &ci) const
{
  if (task() != task_t::classification)
    return ci.domain();

  return get_index(ci, cols_) > 0 ? ci.domain() : d_double;
}

symbol::category_t columns_info::category(const column_info &ci) const
{
  const auto target(get_index(ci, cols_));

  std::vector<symbol::category_t> ret;
  ret.reserve(target + 1);

  // This value isn't always equal to `ret.size()` because of possible columns
  // with `undefined_category`.
  std::size_t found_categories(0);

  for (std::size_t i(0); i <= target; ++i)  // identifying `i`-th column
  {
    const domain_t d(evaluation_domain(cols_[i]));

    auto id(symbol::undefined_category);
    if (d == d_void)
      ;
    else if (typing_ == typing::strong || d == d_string)
      id = found_categories++;
    else  // weak typing
    {
      for (std::size_t j(0); j < i; ++j)
        if (evaluation_domain(cols_[j]) == d)
        {
          id = ret[j];
          break;
        }

      if (id == symbol::undefined_category)  // same domain column not seen yet
        id = found_categories++;
    }

    ret.push_back(id);
  }

  Ensures(ret.size() == target + 1);
  return ret[target];
}

///
/// \return a set of all categories used across columns
///
std::set<symbol::category_t> columns_info::used_categories() const
{
  std::set<symbol::category_t> categories;

  for (std::size_t column(0); column < size();  ++column)
    categories.insert(category(cols_[column]));

  return categories;
}

domain_t columns_info::domain_of_category(symbol::category_t target) const
{
  const auto it(std::ranges::find_if(
                  cols_,
                  [target](const auto &ci) {return ci.category() == target;}));

  return it == cols_.end() ? d_void : evaluation_domain(*it);
}

///
/// \return `true` if all columns have valid domains and consistent categories
///
bool columns_info::is_valid() const
{
  for (std::size_t i(0); i < cols_.size(); ++i)
  {
    const auto &c(cols_[i]);

    if (!basic_data_type(c.domain()))
      return false;

    if (c.domain() == d_void && !c.states().empty())
      return false;

    if (std::ranges::any_of(c.states(),
                            [&c](const auto &v)
                            { return v.index() != c.domain(); }))
      return false;

    const auto cat(c.category());
    const auto dom(evaluation_domain(c));
    for (std::size_t j(i + 1); j < cols_.size(); ++j)
      if (cat == cols_[j].category() && dom != evaluation_domain(cols_[j]))
        return false;
  }

  return true;
}

}  // namespace ultra
