/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/gp/gene.h"

namespace ultra
{

///
/// Utility constructor to input hard-coded genomes.
///
/// \param[in] f pointer to a function
/// \param[in] a arguments of the function
///
/// A constructor that makes it easy to write genome "by hand":
///     std::vector<gene> g(
///     {
///       {f_add, {1, 2}},  // [0] ADD 1,2
///       {    y,     {}},  // [1] Y
///       {    x,     {}}   // [2] X
///     };
///
gene::gene(const function *f, const arg_pack &a) : func(f), args(a)
{
  Expects(f);
  Expects(a.size() == f->arity());
}

bool gene::is_valid() const
{
  if (!func)
    return args.empty();

  return func->arity() == args.size();
}

}  // namespace ultra
