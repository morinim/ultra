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
#include "kernel/gp/src/dataframe_summary.h"
#include "kernel/gp/src/variable.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>

namespace ultra::src
{

namespace internal
{

///
/// Checks if a sequence of categories matches a pattern of category names
/// and/or domain names.
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
/// \warning
/// Column names that coincide with domain identifiers (such as "numeric",
/// "string", or "integer") cannot be distinguished from domain specifications
/// and will always be interpreted as domains.
///
bool compatible(const function::param_data_types &instance,
                const std::vector<std::string> &pattern,
                const columns_info &columns)
{
  Expects(instance.size() == pattern.size());

  const auto sup(instance.size());
  for (std::size_t i(0); i < sup; ++i)
  {
    const std::string &p_i(pattern[i]);
    const auto domain(from_weka(p_i));
    const bool generic(domain != d_void);

    if (generic)  // numeric, string, integer...
    {
      if (columns.domain_of_category(instance[i]) != domain)
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

[[nodiscard]] double ephemeral_bound(const distribution<double> &dist)
{
  if (dist.empty())
    return 100.0;

  std::map<double, std::uintmax_t> magnitudes;
  for (const auto &[value, count] : dist.seen())
    magnitudes[std::abs(value)] += count;

  // Nearest-rank 95th percentile of the rounded absolute values.
  const auto rank(dist.size() - dist.size() / 20);
  std::uintmax_t cumulative(0);
  constexpr auto real_max(std::numeric_limits<D_DOUBLE>::max() / 2.0);

  for (const auto &[magnitude, count] : magnitudes)
  {
    cumulative += count;

    if (cumulative >= rank)
      return std::clamp(magnitude, 1.0, real_max);
  }

  return 100.0;
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
  ultraINFO("Importing dataset...");
  data[dataset_t::training] = std::move(d);
  ultraINFO("...dataset imported");

  data[dataset_t::validation].clone_schema(data[dataset_t::training]);
  data[dataset_t::test].clone_schema(data[dataset_t::training]);

  if (init_flags != symbol_init::disabled)
    setup_symbols(init_flags);

  ultraINFO("Examples: {}, features: {}, classes: {}, categories: {}",
              data[dataset_t::training].size(), variables(), classes(),
              categories());
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
/// Constructs a src::problem from an existing base problem by transferring
/// configuration and symbol set ownership.
///
/// \param[in,out] base source problem to consume (moved-from on return)
/// \param[in]     d    training dataset for the new problem
///
/// \post
/// - `this->params` and `this->sset` are taken from `base`
/// - `base` remains valid but its `params`/`sset` are in a moved-from state
///
/// \warning
/// This constructor does not prove semantic compatibility between `d` and the
/// moved symbol set. The caller must ensure categories/domains are coherent.
///
problem::problem(ultra::problem &&base, dataframe d)
  : problem(std::move(d), symbol_init::disabled)
{
  using std::swap;
  swap(params, base.params);
  swap(sset, base.sset);

  Ensures(is_valid());
}

///
/// Checks whether the problem can be used to start an evolutionary run.
///
/// \return success if the problem is ready, otherwise the first detected
///         reason it is not ready
///
/// A problem is considered ready when:
/// - a non-empty training dataset is available;
/// - the symbol set contains a sufficient number of terminals.
/// - the symbol set contains functions.
/// - each category used by the training data has at least one terminal.
///
std::expected<void, problem::readiness_error> problem::ready() const
{
  if (!params.is_valid(false))
    return std::unexpected(readiness_error::invalid_problem);

  for (auto i : {dataset_t::training, dataset_t::validation, dataset_t::test})
    if (!data[i].is_valid())
      return std::unexpected(readiness_error::invalid_problem);

  const auto &training(data[dataset_t::training]);

  if (training.empty())
    return std::unexpected(readiness_error::empty_training_set);

  if (!sset.enough_terminals())
    return std::unexpected(readiness_error::insufficient_terminals);

  if (!sset.is_valid())
    return std::unexpected(readiness_error::invalid_problem);

  if (!sset.functions())
    return std::unexpected(readiness_error::missing_functions);

  for (auto category : training.columns.used_categories())
    if (category != symbol::undefined_category && !sset.terminals(category))
      return std::unexpected(readiness_error::missing_category_terminals);

  return {};
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
  ultraINFO("Setting up terminals...");

  const auto &ds(data[dataset_t::training]);
  const auto &columns(ds.columns);

  if (columns.size() <= 1)
    throw exception::insufficient_data("Cannot generate the terminal set");

  // ********* Sets up variables *********
  if (has_flag(init_flags, symbol_init::variables))
  {
    std::map<symbol::category_t, std::string> variables;
    for (std::size_t i(1); i < columns.size(); ++i)
    {
      const auto category(columns[i].category());
      if (columns[i].domain() == d_void
          || category == symbol::undefined_category)
        continue;

      // Sets up the variables (features).
      const auto name(columns[i].name().empty() ? "X" + std::to_string(i)
                                                : columns[i].name());

      if (insert<variable>(i - 1, name, category))
        variables[category] += " `" + name + "`";
    }

    for (const auto &[category, inserted] : variables)
      if (!inserted.empty())
      {
        ultraINFO("Category {} variables:{}", category, inserted);
      }
  }

  // ********* Sets up nominal attributes *********
  if (has_flag(init_flags, symbol_init::attributes))
  {
    std::map<symbol::category_t, std::set<value_t>> attributes;
    for (std::size_t i(1); i < columns.size(); ++i)
    {
      const auto category(columns[i].category());
      if (columns[i].domain() == d_void
          || category == symbol::undefined_category)
        continue;

      for (const auto &s : columns[i].states())
        if (!attributes[category].contains(s))
        {
          bool ok(false);

          switch (columns[i].domain())
          {
          case d_double:
            ok = insert<real::literal>(std::get<D_DOUBLE>(s), category);
            break;
          case d_int:
            ok = insert<integer::literal>(std::get<D_INT>(s), category);
            break;
          case d_string:
            ok = insert<str::literal>(std::get<D_STRING>(s), category);
            break;
          default:
            ultraWARNING("Attribute '{}' from column `{}` not inserted",
                         lexical_cast<std::string>(s), columns[i].name());
          }

          if (ok)
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

        ultraINFO("Category {} attributes: {}", category,
                    attributes_in_category);
      }
  }

  if (has_flag(init_flags, symbol_init::ephemerals))
  {
    std::vector<double> category_bounds(columns.size(), 0.0);

    if (ds.task() == task_t::regression)
    {
      const dataframe_summary sum(ds);

      for (std::size_t i(1); i < columns.size(); ++i)
        if (const auto *stats = std::get_if<numeric_stats>(&sum[i].stats);
            stats && !stats->dist.empty())
        {
          auto &bound(category_bounds[columns[i].category()]);
          bound = std::max(bound, internal::ephemeral_bound(stats->dist));
        }
    }

    for (auto category : columns.used_categories())
    {
      const auto domain(columns.domain_of_category(category));
      if (!numerical_data_type(domain))
        continue;

      const auto category_bound(category_bounds[category]);
      const auto M(category_bound > 0.0 ? category_bound : 100.0);

      switch (domain)
      {
      case d_double:
        if (const auto *t = insert<real::number>(-M, M, category))
          ultraINFO("Category {} ephemeral `{}` interval [{}, {})",
                    category, t->name(), t->min(), t->sup());
        break;

      case d_int:
      {
        constexpr auto d_int_max(std::numeric_limits<D_INT>::max());
        const auto bound(
          static_cast<D_INT>(std::min(std::ceil(M), static_cast<double>(d_int_max))));
        const auto sup(ds.task() == task_t::regression && bound < d_int_max
                       ? bound + 1 : bound);

        if (const auto *t = insert<integer::number>(-bound, sup, category))
          ultraINFO("Category {} ephemeral `{}` interval [{}, {})",
                    category, t->name(), t->min(), t->sup());
        break;
      }

      default:
        break;
      }
    }
  }

  ultraINFO("...terminals ready");
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
///   impossible to determine, among other things, the dataset's features.
/// - Multi-category tasks are supported, but the result may be suboptimal.
///
void problem::setup_symbols(symbol_init init_flags)
{
  ultraINFO("Automatically setting up symbol set...");

  if (sset.terminals())
    ultraWARNING("Terminals already present, initialisation skipped");
  else
    setup_terminals(init_flags);

  if (sset.functions())
    ultraWARNING("Functions already present, initialisation skipped");
  else if (has_flag(init_flags, symbol_init::functions))
  {
    ultraINFO("Setting up functions...");

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
      if (category == symbol::undefined_category)
        continue;

      const auto domain(
        data[dataset_t::training].columns.domain_of_category(category));

      switch (domain)
      {
      case d_void:
        break;

      case d_double:
        add_symbol(insert<real::add>(category));
        add_symbol(insert<real::div>(
                     category, function::param_data_types{category, category}));
        add_symbol(insert<real::mul>(
                     category, function::param_data_types{category, category}));
        add_symbol(insert<real::sin>(category));
        add_symbol(insert<real::sub>(category));

        // Keep the default classification set small; nonlinear functions can be
        // added explicitly when the data calls for them.
        if (!classification())
        {
          add_symbol(insert<real::ln>(category));
          add_symbol(insert<real::exp>(category));
        }
        break;

      case d_int:
        add_symbol(insert<integer::add>(category));
        add_symbol(insert<integer::sub>(category));
        add_symbol(insert<integer::mul>(
                     category, function::param_data_types{category, category}));
        add_symbol(insert<integer::div>(
                     category, function::param_data_types{category, category}));
        break;

      case d_string:
        add_symbol(insert<str::ife>(
                     symbol::default_category,
                     function::param_data_types{category, category,
                                                symbol::default_category,
                                                symbol::default_category}));
        break;

      default:
        ultraWARNING("No predefined functions for domain {} (category {})",
                     std::to_underlying(domain), category);
        break;
      }
    }

    for (const auto &[category, names] : symbols)
      if (!names.empty())
        ultraINFO("Category {} symbols:{}", category, names);

    ultraINFO("...functions ready");
  }

  ultraINFO("...symbol set setup complete");
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
