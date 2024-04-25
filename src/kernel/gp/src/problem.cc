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

#include <set>

#include "kernel/gp/src/problem.h"
#include "kernel/exceptions.h"
#include "kernel/gp/function.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/primitive/string.h"
#include "kernel/gp/src/variable.h"

namespace ultra::src
{

///
/// Initializes problem dataset with examples coming from a file.
///
/// \param[in] ds name of the dataset file (CSV or XRFF format)
/// \param[in] p  additional, optional, parameters (see `dataframe::params`
///               structure)
///
/// \warning
/// - Users **must** specify, at least, the functions to be used;
/// - terminals directly derived from the data (variables / labels) are
///   automatically inserted;
/// - any additional terminal (ephemeral random constant, problem specific
///   constant...) can be manually inserted.
///
problem::problem(const std::filesystem::path &ds, const dataframe::params &p)
  : problem()
{
  ultraINFO << "Reading dataset " << ds << "...";

  training_.read(ds, p);

  setup_terminals();

  ultraINFO << "...dataset read."
            << " Examples: " << training_.size()
            << ", categories: " << categories()
            << ", features: " << variables()
            << ", classes: " << classes();
}

///
/// Initializes problem dataset with examples coming from a file.
///
/// \param[in] ds dataset
/// \param[in] p  additional, optional, parameters (see `dataframe::params`
///               structure)
///
/// \warning
/// - Users **must** specify, at least, the functions to be used;
/// - terminals directly derived from the data (variables / labels) are
///   automatically inserted;
/// - any additional terminal (ephemeral random constant, problem specific
///   constants...) can be manually inserted.
///
problem::problem(std::istream &ds, const dataframe::params &p) : problem()
{
  ultraINFO << "Reading dataset from input stream...";

  training_.read_csv(ds, p);

  setup_terminals();

  ultraINFO << "...dataset read."
            << " Examples: " << training_.size()
            << ", categories: " << categories()
            << ", features: " << variables()
            << ", classes: " << classes();
}

///
/// \return `false` if the current problem isn't ready for a run
///
bool problem::operator!() const
{
  return !training_.size() || !sset.enough_terminals();
}

///
/// Inserts variables and states for nominal attributes into the symbol_set.
///
/// \exception `std::data_format` unsupported state domain
///
/// There is a variable for each feature.
///
/// The names used for variables, if not specified in the dataset, are in the
/// form `X1`, ... `Xn`.
///
void problem::setup_terminals()
{
  ultraINFO << "Setting up terminals...";

  const auto &columns(training_.columns);
  if (columns.size() <= 1)
    throw exception::insufficient_data("Cannot generate the terminal set");

  std::string variables;

  for (std::size_t i(1); i < columns.size(); ++i)
  {
    // Sets up the variables (features).
    const auto name(columns[i].name().empty() ? "X" + std::to_string(i)
                                              : columns[i].name());
    const auto category(columns[i].category());

    if (insert<variable>(i - 1, name, category))
      variables += " `" + name + "`";

    // Sets up states for nominal attributes.
    for (const auto &s : columns[i].states())
      switch (columns[i].domain())
      {
      case d_double:
        insert<real::literal>(std::get<D_DOUBLE>(s), category);
        break;
      case d_int:
        insert<integer::literal>(std::get<D_INT>(s), category);
        break;
      case d_string:
        insert<str::literal>(std::get<D_STRING>(s), category);
        break;
      default:
        exception::insufficient_data("Cannot generate the terminal set");
      }
  }

  ultraINFO << "...terminals ready. Variables:" << variables;
}

///
/// Default symbol set.
///
/// \return number of symbols inserted
///
/// A predefined set is arranged (useful for simple problems: single category
/// regression / classification).
///
/// \warning
/// Data should be loaded before symbols: without data we don't know, among
/// other things, the features the dataset has.
///
std::size_t problem::setup_symbols()
{
  ultraINFO << "Setting up default symbol set...";

  const auto used_categories(training_.columns.used_categories());
  std::size_t inserted(0);

  for (const auto &category : used_categories)
    if (compatible({category}, {"numeric"}))
    {
      sset.insert<real::literal>(1.0, category);
      sset.insert<real::literal>(2.0, category);
      sset.insert<real::literal>(3.0, category);
      sset.insert<real::literal>(4.0, category);
      sset.insert<real::literal>(5.0, category);
      sset.insert<real::literal>(6.0, category);
      sset.insert<real::literal>(7.0, category);
      sset.insert<real::literal>(8.0, category);
      sset.insert<real::literal>(9.0, category);

      sset.insert<real::abs>(category);
      sset.insert<real::add>(category);
      sset.insert<real::div>(category,
                             function::param_data_types{category, category});
      sset.insert<real::ln>(category);
      sset.insert<real::mul>(category,
                             function::param_data_types{category, category});
      sset.insert<real::mod>(category,
                             function::param_data_types{category, category});
      sset.insert<real::sub>(category);

      inserted = 16;
    }
    else if (compatible({category}, {"string"}))
    {
      sset.insert<str::ife>(symbol::default_category,
                            function::param_data_types{category});
      inserted = 1;
    }

  ultraINFO << "...default symbol set ready. Symbols: " << inserted;
  return inserted;
}

///
/// Checks if a sequence of categories matches a sequence of domain names.
///
/// \param[in] instance a vector of categories
/// \param[in] pattern  a mixed vector of category names and domain names
/// \return             `true` if `instance` match `pattern`
///
/// For instance:
///
///     category_t km_h, name;
///     compatible({km_h}, {"km/h"}) == true
///     compatible({km_h}, {"numeric"}) == true
///     compatible({km_h}, {"string"}) == false
///     compatible({km_h}, {"name"}) == false
///     compatible({name}, {"string"}) == true
///
bool problem::compatible(const function::param_data_types &instance,
                         const std::vector<std::string> &pattern) const
{
  Expects(instance.size() == pattern.size());

  const auto &columns(training_.columns);

  const auto sup(instance.size());
  for (std::size_t i(0); i < sup; ++i)
  {
    const std::string p_i(pattern[i]);
    const bool generic(from_weka(p_i) != d_void);

    if (generic)  // numeric, string, integer...
    {
      if (columns.domain_of_category(instance[i]) != from_weka(p_i))
        return false;
    }
    else
    {
      if (instance[i] != columns[p_i].category())
        return false;
    }
  }

  return true;
}

///
/// \return number of categories of the problem (`>= 1`)
///
std::size_t problem::categories() const noexcept
{
  return sset.categories();
}

///
/// \return number of classes of the problem (`== 0` for a symbolic regression
///         problem, `> 1` for a classification problem)
///
std::size_t problem::classes() const noexcept
{
  return training_.classes();
}

///
/// \return dimension of the input vectors (i.e. the number of variable of
///         the problem)
///
std::size_t problem::variables() const noexcept
{
  return training_.variables();
}

///
/// \param[in] t a dataset type
/// \return      a reference to the specified dataset
///
dataframe &problem::data(dataset_t t) noexcept
{
  return t == dataset_t::training ? training_ : validation_;
}

///
/// \param[in] t a dataset type
/// \return      a const reference to the specified dataset
///
const dataframe &problem::data(dataset_t t) const noexcept
{
  return t == dataset_t::training ? training_ : validation_;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool problem::is_valid() const
{
  return ultra::problem::is_valid()
         && training_.is_valid() && validation_.is_valid();
}

}  // namespace ultra::src
