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
#include "utility/misc.h"

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

///
/// \param[in] i ordinal of an argument
/// \return      the locus that `i`-th argument of the current function refers to
///
locus gene::locus_of_argument(std::size_t i) const
{
  Expects(i < func->arity());
  Expects(std::holds_alternative<D_ADDRESS>(args[i]));

  return {static_cast<locus::index_t>(std::get<D_ADDRESS>(args[i])),
          func->categories(i)};
}

///
/// \param[in] a an argument
/// \return      the locus that `a` argument refers to
///
locus gene::locus_of_argument(const arg_pack::value_type &a) const
{
  Expects(std::holds_alternative<D_ADDRESS>(a));

  return {
           static_cast<locus::index_t>(std::get<D_ADDRESS>(a)),
           func->categories(get_index(a, args))
         };
}

symbol::category_t gene::category() const
{
  Expects(func);
  return func->category();
}

bool gene::is_valid() const
{
  return (func && func->arity() == args.size()) || (!func && args.empty());
}

}  // namespace ultra
