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

#include <algorithm>

#include "kernel/gp/src/columns_info.h"
#include "utility/assert.h"
#include "utility/misc.h"

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
/// The category of the column.
///
/// \note
/// This is a computed field: if the value have to be used multiple time store
/// it in a local variable (buffer).
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
/// Copy assignment operator.
///
/// \param[in] other another container to use as data source
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

columns_info::column_info columns_info::operator[](
  const std::string &name) const
{
  const auto it(std::ranges::find_if(
                  cols_,
                  [name](const auto &c)
                  {
                    return c.name() == name;
                  }));
  return it == cols_.end() ? column_info(*this) : *it;
}

///
/// Set the data typing system used for category identification.
///
/// \param[in] t king of data typing (see `typing` for details)
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
/// Removes the last element of the container.
///
/// \warning
/// Calling pop_back on an empty container results in undefined behavior.
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
/// Adds a new column at the front of the column list.
///
/// \param[in] v information about the new column
///
void columns_info::push_front(const column_info &v)
{
  cols_.insert(begin(), v);
}

///
/// Given an example compiles information about the columns of the dataframe.
///
/// \param[in] r            a record containing an example
/// \param[in] header_first `true` if the first example contains the header
///
/// The function can be called multiple times to incrementally collect
/// information from different examples.
///
/// When `header_first` is `true` the first example is used to gather the names
/// of the columns and successive example contribute to determine the domain
/// of each column.
///
/// \remark
/// The function assumes columns `0` as the output column.
///
void columns_info::build(const std::vector<std::string> &r, bool header_first)
{
  Expects(r.size());

  // Sets the domain associated to a column.
  const auto set_domain(
    [&](std::size_t idx)
    {
      const std::string &value(trim(r[idx]));
      if (value.empty())
        return;

      const bool number(is_number(value));
      const bool classification(idx == 0 && !number);

      // DOMAIN
      if (cols_[idx].domain() == d_void)
        // For classification tasks we use discriminant functions and the actual
        // output type is always numeric.
        cols_[idx].domain(number || classification ? d_double : d_string);
    });

  const auto fields(r.size());

  if (cols_.empty())
  {
    cols_.reserve(fields);

    if (header_first)  // first line contains the names of the columns
    {
      std::ranges::transform(r, std::back_inserter(cols_),
                             [this](const auto &name)
                             {
                               return column_info(*this, trim(name));
                             });

      return;
    }
    else
      std::fill_n(std::back_inserter(cols_), fields, column_info(*this));
  }

  assert(size() == r.size());

  for (std::size_t field(0); field < fields; ++field)
    set_domain(field);
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
    auto id(symbol::undefined_category);
    if (cols_[i].domain() == d_void)
      ;
    else if (typing_ == typing::strong || cols_[i].domain() == d_string)
      id = found_categories++;
    else  // weak typing
    {
      for (std::size_t j(0); j < i; ++j)
        if (cols_[j].domain() == cols_[i].domain())
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
/// \return the set of used categories
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

  return it == cols_.end() ? d_void : it->domain();
}

///
/// \return `true` if the object passes the internal consistency check
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
    for (std::size_t j(i + 1); j < cols_.size(); ++j)
      if (cat == cols_[j].category() && c.domain() != cols_[j].domain())
        return false;
  }

  return true;
}

}  // namespace ultra
