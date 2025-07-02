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

#include "kernel/gp/src/problem.h"
#include "kernel/exceptions.h"
#include "kernel/gp/function.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/primitive/string.h"
#include "kernel/gp/src/variable.h"

#include <set>

namespace ultra::src
{

namespace internal
{

///
/// Checks if a sequence of categories matches a sequence of domain names.
///
/// \param[in] instance a sequence of categories
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
bool compatible(const function::param_data_types &instance,
                const std::vector<std::string> &pattern,
                const columns_info &columns)
{
  Expects(instance.size() == pattern.size());

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

}  // namespace internal

problem::problem(dataframe d)
{
  ultraINFO << "Importing dataset...";
  data[dataset_t::training] = std::move(d);
  ultraINFO << "...dataset imported";

  ultraINFO << "Examples: " << data[dataset_t::training].size()
            << ", features: " << variables()
            << ", classes: " << classes()
            << ", categories: " << categories();


  data[dataset_t::validation].clone_schema(data[dataset_t::training]);
  data[dataset_t::test].clone_schema(data[dataset_t::training]);

  setup_terminals();
}

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
  : problem(dataframe(ds, p))
{
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
problem::problem(std::istream &ds, const dataframe::params &p)
  : problem(dataframe(ds, p))
{
}

///
/// \return `false` if the current problem isn't ready for a run
///
bool problem::operator!() const
{
  return !data[dataset_t::training].size() || !sset.enough_terminals();
}

///
/// Inserts variables and states for nominal attributes into the symbol_set.
///
/// \exception `std::insufficient_data` not enough data to generate a terminal
///                                     set
///
/// There is a variable for each feature.
///
/// The names used for variables, if not specified in the dataset, are in the
/// form `X1`, ... `Xn`.
///
void problem::setup_terminals()
{
  ultraINFO << "Setting up terminals...";

  const auto &columns(data[dataset_t::training].columns);
  if (columns.size() <= 1)
    throw exception::insufficient_data("Cannot generate the terminal set");

  // ********* Sets up variables *********
  std::map<symbol::category_t, std::string> variables;
  for (std::size_t i(1); i < columns.size(); ++i)
  {
    // Sets up the variables (features).
    const auto name(columns[i].name().empty() ? "X" + std::to_string(i)
                                              : columns[i].name());
    const auto category(columns[i].category());

    if (insert<variable>(i - 1, name, category))
      variables[category] += " `" + name + "`";
  }

  for (const auto &[category, inserted] : variables)
    if (!inserted.empty())
    {
      ultraINFO << "Category " << category << " variables:" << inserted;
    }

  // ********* Sets up nominal attributes *********
  std::map<symbol::category_t, std::set<value_t>> attributes;
  for (std::size_t i(1); i < columns.size(); ++i)
  {
    const auto category(columns[i].category());

    if (!attributes.contains(category))
      attributes[category] = {};

    for (const auto &s : columns[i].states())
    {
      if (attributes[category].contains(s))
        continue;

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

      attributes[category].insert(s);
    }
  }

  for (const auto &[category, inserted] : attributes)
    if (!inserted.empty())
    {
      std::string attributes_in_category;

      for (const auto &attribute : inserted)
        attributes_in_category += " `" + lexical_cast<std::string>(attribute)
                                  + "`";

      ultraINFO << "Category " << category << " attributes: "
                << attributes_in_category;
    }

  ultraINFO << "...terminals ready";
}

///
/// Automatic set up of a symbol set.
///
/// A predefined set is arranged (useful for simple problems: single category
/// regression / classification).
///
/// Multi-category tasks are supported but the result is suboptimal.
///
/// \warning
/// Data should be loaded before symbols: without data we don't know, among
/// other things, the features of the dataset.
///
void problem::setup_symbols()
{
  ultraINFO << "Automatically setting up symbol set...";

  std::map<symbol::category_t, std::string> symbols;
  const auto add_symbol([&symbols](symbol *s)
  {
    if (s)
      symbols[s->category()] += " `" + s->name() + "`";
  });

  if (!sset.terminals())
    setup_terminals();

  for (const auto used_categories(
         data[dataset_t::training].columns.used_categories());
       auto category : used_categories)
  {
    if (const auto domain(
          data[dataset_t::training].columns.domain_of_category(category));
        numerical_data_type(domain))
    {
      add_symbol(insert<real::abs>(category));
      add_symbol(insert<real::add>(category));
      add_symbol(insert<real::div>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::ln>(category));
      add_symbol(insert<real::mul>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::mod>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::sub>(category));
    }
    else if (domain == d_string)
    {
      add_symbol(insert<str::ife>(
                   symbol::default_category,
                   function::param_data_types{category, category,
                                              symbol::default_category,
                                              symbol::default_category}));
    }
  }

  for (const auto categories(sset.categories_missing_terminal());
       auto category : categories)
    if (const auto domain(
          data[dataset_t::training].columns.domain_of_category(category));
        numerical_data_type(domain))
    {
      if (domain == d_double)
        add_symbol(insert<real::number>(-100.0, 100.0, category));
      else
        add_symbol(insert<integer::number>(-100, 100, category));
    }

  for (const auto &[category, names] : symbols)
    if (!names.empty())
    {
      ultraINFO << "Category " << category << " symbols:" << names;
    }

  ultraINFO << "...symbol set ready";
}

///
/// Checks if a sequence of categories matches a sequence of domain names.
///
/// \param[in] instance a sequence of categories
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
  return internal::compatible(instance, pattern,
                              data[dataset_t::training].columns);
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
  return data[dataset_t::training].classes();
}

///
/// \return dimension of the input vectors (i.e. the number of variable of
///         the problem)
///
std::size_t problem::variables() const noexcept
{
  return data[dataset_t::training].variables();
}

///
/// \return `true` if the object passes the internal consistency check
///
bool problem::is_valid() const
{
  if (!ultra::problem::is_valid())
    return false;

  for (auto i : std::vector{dataset_t::training, dataset_t::validation,
                            dataset_t::test})
    if (!data[i].is_valid())
      return false;

  return true;
}

}  // namespace ultra::src
