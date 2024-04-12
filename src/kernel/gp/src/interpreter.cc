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

#include "kernel/gp/src/interpreter.h"

namespace ultra::src
{

///
/// \param[in] prg individual whose value we are interested in
///
/// \warning
/// The lifetime of `prg` must extend beyond that of the interpreter.
///
interpreter::interpreter(const gp::individual &prg) : ultra::interpreter(prg)
{
}

///
/// Calculates the output of a program (individual) given a specific input.
///
/// \param[in] ex a vector of values for the problem's variables
/// \return       the output value of the interpreter
///
value_t interpreter::run(const std::vector<value_t> &ex)
{
  example_ = &ex;
  return this->run();
}

///
/// Used by the ultra::variable class to retrieve the value of a variable.
///
/// \param[in] i the index of a variable
/// \return      the value of the `i`-th variable
///
value_t interpreter::fetch_var(std::size_t i) const
{
  Expects(i < example_->size());
  return (*example_)[i];
}

///
/// A handy short-cut for one-time execution of an individual.
///
/// \param[in] ind individual/program to be run
/// \param[in] ex  input values for the current training case (used to valorize
///                the variables of `ind`)
/// \return        possible output value of the individual
///
value_t run(const gp::individual &ind, const std::vector<value_t> &ex)
{
  return interpreter(ind).run(ex);
}

} // namespace ultra::src
