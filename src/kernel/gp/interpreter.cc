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

#include "kernel/nullary.h"
#include "kernel/gp/interpreter.h"
#include "kernel/gp/src/variable.h"
#include "utility/misc.h"

namespace ultra
{

const gene &interpreter::current_gene() const
{
  return (*prg_)[ip_];
}

///
/// \param[in] ind individual whose value we are interested in
///
/// \warning
/// The lifetime of `ind` must extend beyond that of the interpreter.
///
interpreter::interpreter(const gp::individual &ind)
  : prg_(&ind), cache_(ind.size(), ind.categories())
{
}

///
/// Rebinds the interpreter to a different individual.
///
/// \param[in] ind individual to which the interpreter is rebound
///
/// This function updates the internal program pointer so that the interpreter
/// evaluates the specified individual without reallocating or rebuilding its
/// internal state.
///
/// \pre
/// The supplied individual must be *compatible* with the current interpreter
/// state. In particular:
/// - `ind.size()` must match the number of rows of the internal cache
/// - `ind.categories()` must match the number of columns of the internal cache
///
/// These conditions are enforced by contracts and represent a logic error if
/// violated.
///
/// \note
/// This operation is lightweight and does not reset the instruction pointer or
/// invalidate the cached values. It is intended to be called in hot paths
/// where reconstructing the interpreter would be unnecessarily expensive.
///
/// \warning
/// Rebinding is only safe when the interpreter cache layout depends solely on
/// the program shape. It does not perform any structural validation beyond the
/// stated preconditions.
///
void interpreter::rebind(const gp::individual &ind) noexcept
{
  Ensures(cache_.rows() == ind.size());
  Ensures(cache_.cols() == ind.categories());

  prg_ = &ind;
}

///
/// Executes the associated individual starting from a given locus.
///
/// \param[in] l locus identifying the gene from which execution starts
/// \return      the value produced by evaluating the individual
///
/// This function initialises the interpreter state, clears any cached values,
/// sets the instruction pointer to the specified locus and evaluates the gene
/// located there.
///
value_t interpreter::run(const locus &l)
{
  Expects(l.index < prg_->size());
  Expects(l.category < prg_->categories());

  for (auto &e : cache_)
    e.valid = false;

  ip_ = l;

  return current_gene().func->eval(*this);
}

///
/// \return the output value of `this` individual
///
/// Use the first available locus as starting IP.
///
value_t interpreter::run()
{
  return run(prg_->start());
}

///
/// Fetches the value of the `i`-th argument of the current gene.
///
/// \param[in] i i-th argument of the current gene
/// \return      the required value
///
/// We use a cache to avoid recalculating the same value during the interpreter
/// execution.
/// This means that side effects are not evaluated to date: WE ASSUME
/// REFERENTIAL TRANSPARENCY for all the expressions.
///
/// This function may internally delegate to `fetch_opaque_arg` when caching
/// is not applicable.
///
/// \see
/// - https://en.wikipedia.org/wiki/Referential_transparency
/// - https://en.wikipedia.org/wiki/Memoization
///
value_t interpreter::fetch_arg(std::size_t i) const
{
  const gene &g(current_gene());

  assert(i < g.func->arity());
  if (const auto arg(g.args[i]); arg.index() == d_address)
  {
    auto &elem(cache_(g.locus_of_argument(i)));

    if (!elem.valid)
    {
      elem.value = fetch_opaque_arg(i);
      elem.valid = true;
    }
#if !defined(NDEBUG)
    else  // cache not empty... checking if the cached value is right
    {
      assert(fetch_opaque_arg(i) == elem.value);
    }
#endif

    Ensures(elem.valid);
    return elem.value;
  }

  return fetch_opaque_arg(i);
}

///
/// Fetches the value of an argument without assuming referential transparency.
///
/// \param[in] i  index of the argument to fetch
/// \return       the computed argument value
///
/// This function retrieves the value of the `i`-th argument of the current
/// gene by fully evaluating it, bypassing any memoisation mechanism.
///
/// It must be used for arguments whose evaluation may produce side effects or
/// whose value must always be recomputed.
///
/// ### Behaviour by argument type
/// - *address**: evaluates the referenced gene by temporarily moving the
///   instruction pointer to the target locus;
/// - **nullary symbol**: directly evaluates the symbol;
/// - **variable**: evaluates the variable in the current execution context;
/// - **immediate value**: returned as-is.
///
/// \warning
/// Calling this function may trigger repeated evaluations of the same
/// sub-expression. Prefer `fetch_arg` when referential transparency holds.
///
value_t interpreter::fetch_opaque_arg(std::size_t i) const
{
  const gene &g(current_gene());

  assert(i < g.func->arity());
  const auto arg(g.args[i]);
  switch (arg.index())
  {
  case d_address:
  {
    revert_on_scope_exit _ {ip_};
    ip_ = g.locus_of_argument(i);

    return current_gene().func->eval(*this);
  }

  case d_nullary:
    return std::get<const D_NULLARY *>(arg)->eval();

  case d_variable:
    return std::get<const D_VARIABLE *>(arg)->eval(*this);

  default:
    return arg;
  }
}

///
/// \return the program associated with this interpreter
///
const gp::individual &interpreter::program() const noexcept
{
  Expects(prg_);

  return *prg_;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool interpreter::is_valid() const
{
  if (!prg_)
    return false;

  return ip_.index < prg_->size() && ip_.category < prg_->categories();
}

///
/// Executes the associated individual starting from its default entry point.
///
/// \param[in] ind individual/program to be run
/// \return        the value produced by evaluating the individual
///
/// This overload uses the individual's first available locus as the initial
/// instruction pointer.
///
value_t run(const gp::individual &ind)
{
  return interpreter(ind).run();
}

}  // namespace ultra
