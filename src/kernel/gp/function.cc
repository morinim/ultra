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

#include "kernel/gp/function.h"

namespace ultra
{
///
/// Constructs a typed function.
///
/// \param[in] name string representation of the function (e.g. "ADD", "+")
/// \param[in] r    return category of the function
/// \param[in] par  categories of the input parameters
///
/// This constructor is typically used in strongly typed GP, where both
/// argument and return categories are explicitly specified.
///
function::function(const std::string &name, category_t r, param_data_types par)
  : symbol(name, r), params_(std::move(par))
{
  Ensures(is_valid());
}

///
/// Constructs an untyped function with the given arity.
///
/// \param[in] name   string representation of the function
/// \param[in] n_pars number of input parameters
///
/// \remark
/// The function and all its parameters are assigned
/// `symbol::default_category`. This constructor is intended for GP
/// configurations where strong typing is not used.
///
function::function(const std::string &name, std::size_t n_pars)
  : function(name, symbol::default_category,
             param_data_types(n_pars, symbol::default_category))
{
}

///
/// Returns the number of input parameters of the function.
///
/// \return function arity
///
std::size_t function::arity() const noexcept
{
  Expects(params_.size());
  return params_.size();
}

///
/// Returns the category of a specific input parameter.
///
/// \param[in] i index of the input parameter
/// \return      category of the `i`-th parameter
///
/// \pre `i < arity()`
///
symbol::category_t function::param_category(std::size_t i) const noexcept
{
  Expects(i < arity());
  return categories()[i];
}

///
/// Returns the list of input parameter categories.
///
/// \return constant reference to the parameter category list
///
/// \remark
/// The size of the returned container is equal to the function arity.
///
const function::param_data_types &function::categories() const noexcept
{
  return params_;
}

///
/// \return the name of the function
///
/// \warning
/// Specific functions have to specialize this method to support different
/// output formats.
///
std::string function::to_string(format) const
{
  std::string args("{0}");
  for (std::size_t i(1); i < arity(); ++i)
    args += ",{" + std::to_string(i) + "}";

  return name() + "(" + args + ")";
}

///
/// Performs an internal consistency check.
///
/// \return `true` if the function satisfies basic validity constraints
///
bool function::is_valid() const
{
  if (!arity())  // this is a function, we want some argument...
    return false;

  return symbol::is_valid();
}

}  // namespace ultra
