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
/// \param[in] name string representation of the function (e.g. for the plus
///                 function it could by "ADD" or "+")
/// \param[in] r    return type of the function (i.e. the category of the
///                 output value)
/// \param[in] par  input parameters (type and number) of the function
///
function::function(const std::string &name, category_t r, param_data_types par)
  : symbol(name, r), params_(std::move(par))
{
  Ensures(is_valid());
}

///
/// \param[in] name   string representation of the function (e.g. for the plus
///                   function it could by "ADD" or "+")
/// \param[in] n_pars number of parameters of the function
///
/// Assumes category `symbol::default_category` for the function and its
/// arguments. This constructor is usually called when types are not used.
///
function::function(const std::string &name, std::size_t n_pars)
  : function(name, symbol::default_category,
             param_data_types(n_pars, symbol::default_category))
{
}

///
/// \return the number arguments of a funtion
///
std::size_t function::arity() const
{
  Expects(params_.size());
  return params_.size();
}

///
/// \param[in] i index of a input parameter
/// \return      category of the `i`-th input parameter
///
symbol::category_t function::categories(std::size_t i) const
{
  Expects(i < arity());
  return categories()[i];
}

///
/// \return list of the categories of the input parameters
///
const function::param_data_types &function::categories() const
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
/// \return `true` if the object passes the internal consistency check
///
bool function::is_valid() const
{
  if (!arity())  // this is a function, we want some argument...
    return false;

  return symbol::is_valid();
}

}  // namespace ultra
