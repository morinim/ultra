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
/// Constructs an active gene.
///
/// \param[in] f pointer to the function symbol
/// \param[in] a arguments list for the function
///
/// \pre `f` must not be null
/// \pre `a.size() == f->arity()`
///
/// This constructor is primarily intended for debugging and hand-crafted
/// individuals:
///
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
/// Returns the locus referenced by the `i`-th argument.
///
/// \param[in] i index of the argument
/// \return      the locus addressed by argument `i`
///
/// \pre `func != nullptr`
/// \pre `i < func->arity()`
/// \pre `args[i]` holds a `D_ADDRESS`
///
/// The returned locus uses the category required by the function for the
/// `i`-th argument.
///
locus gene::locus_of_argument(std::size_t i) const
{
  Expects(i < func->arity());
  Expects(std::holds_alternative<D_ADDRESS>(args[i]));

  return {static_cast<locus::index_t>(std::get<D_ADDRESS>(args[i])),
          func->categories(i)};
}

///
/// Returns the locus referenced by an argument value.
///
/// \param[in] a an argument belonging to this gene
/// \return      the locus addressed by `a`
///
/// \pre `func != nullptr`
/// \pre `a` holds a `D_ADDRESS`
/// \pre `a` must belong to this gene's argument list
///
/// The argument position is inferred from the argument list in order to
/// recover the correct category.
///
locus gene::locus_of_argument(const arg_pack::value_type &a) const
{
  Expects(std::holds_alternative<D_ADDRESS>(a));

  return {
           static_cast<locus::index_t>(std::get<D_ADDRESS>(a)),
           func->categories(get_index(a, args))
         };
}

///
/// Returns the output category of the gene.
///
/// \return the category of the function represented by this gene
///
/// \pre `func != nullptr`
///
symbol::category_t gene::category() const
{
  Expects(func);
  return func->category();
}

///
/// Checks the internal consistency of the gene.
///
/// \return `true` if the gene satisfies its invariants
///
/// This function verifies that the gene is either empty or fully initialised
/// with a matching function and argument list.
///
bool gene::is_valid() const noexcept
{
  return (func && func->arity() == args.size()) || (!func && args.empty());
}

}  // namespace ultra
