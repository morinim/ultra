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

//#include "third_party/tinyxml2/tinyxml2.h"

namespace ultra::src
{

namespace implementation
{

///
/// \param[in] availables the "dictionary" for the sequence
/// \param[in] size       length of the output sequence
/// \return               a set of sequences with repetition with elements
///                       taken from a given set (`availables`) and fixed
///                       length (`size`).
///
/// \note This is in the `detail` namespace for ease of testing.
///
template<class C>
[[nodiscard]] std::set<std::vector<C>> seq_with_rep(
  const std::set<C> &availables, std::size_t size)
{
  Expects(availables.size());
  Expects(size);

  std::set<std::vector<C>> ret;

  std::function<void (std::size_t, std::vector<C>)> swr;
  swr = [&](std::size_t left, std::vector<C> current)
        {
          if (!left)  // we have a sequence of the correct length
          {
            ret.insert(current);
            return;
          }

          for (auto elem : availables)
          {
            current.push_back(elem);
            swr(left - 1, current);
            current.pop_back();
          }
        };

  swr(size, {});
  return ret;
}

}  // namespace implementation

///
/// Initializes problem dataset with examples coming from a file.
///
/// \param[in] ds name of the dataset file (CSV or XRFF format)
/// \param[in] t  weak or strong data typing
///
/// \warning
/// - Users **must** specify, at least, the functions to be used;
/// - terminals directly derived from the data (variables / labels) are
///   automatically inserted;
/// - any additional terminal (ephemeral random constant, problem specific
///   constant...) can be manually inserted.
///
problem::problem(const std::filesystem::path &ds, typing t) : problem()
{
  ultraINFO << "Reading dataset " << ds << "...";

  dataframe::params p;
  p.data_typing = t;
  training_.read(ds, p);

  ultraINFO << "...dataset read."
            << " Examples: " << training_.size()
            << ", categories: " << categories()
            << ", features: " << variables()
            << ", classes: " << classes();

  setup_terminals();
}

///
/// Initializes problem dataset with examples coming from a file.
///
/// \param[in] ds dataset
/// \param[in] t  weak or strong typing
///
/// \warning
/// - Users **must** specify, at least, the functions to be used;
/// - terminals directly derived from the data (variables / labels) are
///   automatically inserted;
/// - any additional terminal (ephemeral random constant, problem specific
///   constants...) can be manually inserted.
///
problem::problem(std::istream &ds, typing t) : problem()
{
  ultraINFO << "Reading dataset from input stream...";

  dataframe::params p;
  p.data_typing = t;
  training_.read_csv(ds, p);

  ultraINFO << "...dataset read."
            << " Examples: " << training_.size()
            << ", categories: " << categories()
            << ", features: " << variables()
            << ", classes: " << classes();

  setup_terminals();
}

///
/// Initializes the problem with the default symbol set and data coming from a
/// file.
///
/// \param[in] ds name of the dataset file
/// \param[in] t  weak or strong typing
///
/// Mainly useful for simple problems (single category regression /
/// classification) or for the initial approach.
///
problem::problem(const std::filesystem::path &ds, const default_symbols_t &,
                 typing t)
  : problem(ds, std::filesystem::path(), t)
{
}

///
/// Initializes the problem with data / symbols coming from input files.
///
/// \param[in] ds      name of the training dataset file
/// \param[in] symbols name of the file containing the symbols to be used.
/// \param[in] t       weak or strong typing
///
problem::problem(const std::filesystem::path &ds,
                 const std::filesystem::path &symbols, typing t)
  : problem()
{
  ultraINFO << "Reading dataset " << ds << "...";

  dataframe::params p;
  p.data_typing = t;
  training_.read(ds, p);

  ultraINFO << "....dataset read."
            << " Examples: " << training_.size()
            << ", categories: " << categories()
            << ", features: " << variables()
            << ", classes: " << classes();

  setup_symbols(symbols);
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
/// Sets up the symbol set.
///
/// \return number of parsed symbols
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
  return setup_symbols({});
}

///
/// Sets up the symbol set.
///
/// \param[in] file name of the file containing the symbols
/// \return         number of parsed symbols
///
/// If a file isn't specified, a predefined set is arranged (useful for simple
/// problems: single category regression / classification).
///
/// \warning
/// Data should be loaded before symbols: without data we don't know, among
/// other things, the features the dataset has.
///
std::size_t problem::setup_symbols(const std::filesystem::path &file)
{
  sset.clear();

  setup_terminals();

  return file.empty() ? setup_symbols_impl() : setup_symbols_impl(file);
}

///
/// Default symbol set.
///
/// \return number of symbols inserted
///
/// This is useful for simple problems (single category regression /
/// classification).
///
std::size_t problem::setup_symbols_impl()
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
/// Initialize the symbols set reading symbols from a file.
///
/// \param[in] file name of the file containing the symbols
/// \return         number of parsed symbols
///
/// \exception exception::data_format wrong data format for symbol file
///
std::size_t problem::setup_symbols_impl(const std::filesystem::path &)
{
  return 0;
/*
  ultraINFO << "Reading symbol set " << file << "...";
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(file.string().c_str()) != tinyxml2::XML_SUCCESS)
    throw exception::data_format("Symbol set format error");

  category_set categories(training_.columns);
  const auto used_categories(categories.used_categories());
  std::size_t parsed(0);

  // When I wrote this, only God and I understood what I was doing.
  // Now, God only knows.
  tinyxml2::XMLHandle handle(&doc);
  auto *symbolset(handle.FirstChildElement("symbolset").ToElement());

  if (!symbolset)
    throw exception::data_format("Empty symbol set");

  for (auto *s(symbolset->FirstChildElement("symbol"));
       s;
       s = s->NextSiblingElement("symbol"))
  {
    if (!s->Attribute("name"))
    {
      ultraERROR << "Skipped unnamed symbol in symbolset";
      continue;
    }
    const std::string sym_name(s->Attribute("name"));

    if (const char *sym_sig = s->Attribute("signature")) // single category,
    {                                                    // uniform init
      for (auto category : used_categories)
        if (compatible({category}, {std::string(sym_sig)}, categories))
        {
          const auto n_args(factory_.args(sym_name));
          std::string signature(sym_name + ":");

          for (std::size_t j(0); j < n_args; ++j)
            signature += " " + std::to_string(category);
          ultraDEBUG << "Adding to symbol set " << signature;

          sset.insert(factory_.make(sym_name, cvect(n_args, category)));
        }
    }
    else  // !sym_sig => complex signature
    {
      auto *sig(s->FirstChildElement("signature"));
      if (!sig)
      {
        ultraERROR << "Skipping " << sym_name << " symbol (empty signature)";
        continue;
      }

      std::vector<std::string> args;
      for (auto *arg(sig->FirstChildElement("arg"));
           arg;
           arg = arg->NextSiblingElement("arg"))
      {
        if (!arg->GetText())
        {
          ultraERROR << "Skipping " << sym_name << " symbol (wrong signature)";
          args.clear();
          break;
        }

        args.push_back(arg->GetText());
      }

      // From the list of all the sequences with repetition of `args.size()`
      // elements...
      const auto sequences(implementation::seq_with_rep(used_categories,
                                                        args.size()));

      // ...we choose those compatible with the xml signature of the current
      // symbol.
      for (const auto &seq : sequences)
        if (compatible(seq, args, categories))
        {
          std::string signature(sym_name + ":");
          for (const auto &j : seq)
            signature += " " + std::to_string(j);
          ultraDEBUG << "Adding to symbol set " << signature;

          sset.insert(factory_.make(sym_name, seq));
        }
    }

    ++parsed;
  }

  ultraINFO << "...symbol set read. Symbols: " << parsed;
  return parsed;*/
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
  return problem::is_valid() && training_.is_valid() && validation_.is_valid();
}

}  // namespace ultra::src
