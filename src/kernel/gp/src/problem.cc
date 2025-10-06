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
/// \return             `true` if `instance` matches `pattern`
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

///
/// Initialises the problem with a dataset and a specified set of symbols
///
/// \param[in] d          input dataframe
/// \param[in] init_flags initialisation type (see `symbol_init` enum)
///
/// By default, terminals directly derived from the data (variables / labels)
/// are automatically inserted; any additional terminals (ephemeral random
/// constants, problem-specific constants...) and functions must be inserted
/// manually.
///
problem::problem(dataframe d, symbol_init init_flags)
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

  setup_symbols(init_flags);
}

///
/// Initialises the problem dataset with examples loaded from a file.
///
/// \param[in] ds name of the dataset file (CSV or XRFF format)
/// \param[in] p  additional optional parameters (see `dataframe::params`
///               structure)
///
/// \warning
/// Users **must** also specify the functions to be used.
///
/// \remark
/// Terminals directly derived from the data (variables / labels) are
/// automatically inserted. Any additional terminals (ephemeral random
/// constants, problem specific constants...) can be inserted manually.
///
problem::problem(const std::filesystem::path &ds, const dataframe::params &p)
  : problem(dataframe(ds, p))
{
}

///
/// Initialises the problem dataset with examples loaded from a file.
///
/// \param[in] ds dataset
/// \param[in] p  additional optional parameters (see `dataframe::params`
///               structure)
///
/// \warning
/// Users **must** also specify the functions to be used.
///
/// \remark
/// Terminals directly derived from the data (variables / labels) are
/// automatically inserted. Any additional terminals (ephemeral random
/// constants, problem specific constants...) can be inserted manually.
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
/// Initialises the terminal set according to a given initialisation type.
///
/// \param[in] init_flags initialisation type (see `symbol_init` enum)
///
/// \exception `std::insufficient_data` not enough data to generate a terminal
///                                     set
///
/// There is a variable for each feature. The names used for variables, if not
/// specified in the dataset, are in the `X1`, ... `Xn` form.
///
void problem::setup_terminals(symbol_init init_flags)
{
  ultraINFO << "Setting up terminals...";

  const auto &columns(data[dataset_t::training].columns);
  if (columns.size() <= 1)
    throw exception::insufficient_data("Cannot generate the terminal set");

  // ********* Sets up variables *********
  if (has_flag(init_flags, symbol_init::variables))
  {
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
  }

  // ********* Sets up nominal attributes *********
  if (has_flag(init_flags, symbol_init::attributes))
  {
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
          ultraWARNING << "Attribute '" << s << "' from column `"
                       << columns[i].name() << "` not inserted";
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
  }

  if (has_flag(init_flags, symbol_init::ephemerals))
  {
    for (auto category : columns.used_categories())
    {
      terminal *inserted_terminal(nullptr);
      const auto domain(columns.domain_of_category(category));

      switch (domain)
      {
      case d_double:
        inserted_terminal = insert<real::number>(-100.0, 100.0, category);
        break;

      case d_int:
        inserted_terminal = insert<integer::number>(-100, 100, category);
        break;

      default:
        break;
      }

      if (inserted_terminal)
      {
        ultraINFO << "Category " << category << " ephemeral `"
                  << inserted_terminal->name() << '`';
      }
    }
  }

  ultraINFO << "...terminals ready";
}

///
/// Automatically sets up the symbol set.
///
/// \param[in] init_flags initialisation type (see `symbol_init` enum)
///
/// A predefined set is created, which is useful for simple problems (e.g.
/// single category regression or classification).
///
/// \remark
/// If the terminal set is not empty, it remains unchanged and `init_flags` are
/// ignored. The same rule applies to the function set.
///
/// \warning
/// - Data must be loaded before creating symbols, as without data it is
///   impossibile to determine, among other things, the dataset's features.
/// - Multi-category tasks are supported, but the result may be suboptimal.
///
void problem::setup_symbols(symbol_init init_flags)
{
  ultraINFO << "Automatically setting up symbol set...";

  if (sset.terminals())
  {
    ultraWARNING << "Terminals already present, initialisation skipped";
  }
  else
    setup_terminals(init_flags);

  if (sset.functions())
  {
    ultraWARNING << "Functions already present, initialisation skipped";
    return;
  }

  if (!has_flag(init_flags, symbol_init::functions))
    return;

  ultraINFO << "Setting up functions...";

  std::map<symbol::category_t, std::string> symbols;
  const auto add_symbol([&symbols](symbol *s)
  {
    if (s)
      symbols[s->category()] += " `" + s->name() + "`";
  });

  for (const auto used_categories(
         data[dataset_t::training].columns.used_categories());
       auto category : used_categories)
  {
    const auto domain(
      data[dataset_t::training].columns.domain_of_category(category));

    switch (domain)
    {
    case d_double:
      add_symbol(insert<real::add>(category));
      add_symbol(insert<real::div>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::ln>(category));
      add_symbol(insert<real::mul>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::sin>(category));
      add_symbol(insert<real::sub>(category));
      break;

    case d_int:
      add_symbol(insert<integer::add>(category));
      add_symbol(insert<real::mod>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<integer::mul>(
                   category, function::param_data_types{category, category}));
      add_symbol(insert<real::sub>(category));
      break;

    case d_string:
      add_symbol(insert<str::ife>(
                   symbol::default_category,
                   function::param_data_types{category, category,
                                              symbol::default_category,
                                              symbol::default_category}));
      break;

    default:
      ultraWARNING << "Unable to insert functions for category " << category;
      break;
    }
  }

  for (const auto &[category, names] : symbols)
    if (!names.empty())
    {
      ultraINFO << "Category " << category << " symbols:" << names;
    }

  ultraINFO << "...functions ready";
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
/// \return dimension of the input vectors (i.e. the number of variables in
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
