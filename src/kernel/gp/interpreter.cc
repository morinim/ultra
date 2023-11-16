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

#include "kernel/nullary.h"
#include "kernel/gp/interpreter.h"
#include "utility/misc.h"

namespace ultra
{

const gene &interpreter::current_gene() const
{
  return (*prg_)[ip_];
}

///
/// \param[in] ind individual whose value/ we are interested in
///
/// \warning
/// The lifetime of `ind` must extend beyond that of the interpreter.
///
interpreter::interpreter(const gp::individual &ind)
  : prg_(&ind), cache_(ind.size(), ind.categories())
{
}

///
/// \param[in] l locus of the genome we are starting evaluation from
/// \return      the output value of `this` individual
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
/// Usa the first available locus as starting IP.
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
/// \see
/// - <https://en.wikipedia.org/wiki/Referential_transparency>
/// - <https://en.wikipedia.org/wiki/Memoization>
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

  default:
    return arg;
  }
}

///
/// \return the program associated with this interpreter
///
const gp::individual &interpreter::program() const
{
  return *prg_;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool interpreter::is_valid() const
{
  return ip_.index < prg_->size();
}

///
/// A handy short-cut for one-time execution of an individual.
///
/// \param[in] ind individual/program to be run
/// \return        output value of the individual
///
value_t run(const gp::individual &ind)
{
  return interpreter(ind).run();
}

}  // namespace ultra
