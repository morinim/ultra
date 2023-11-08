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

#include "kernel/gp/individual.h"
#include "kernel/nullary.h"
#include "kernel/random.h"
#include "utility/log.h"
#include "utility/misc.h"

namespace ultra::gp
{

///
/// Generates the initial, random expressions that make up an individual.
///
/// \param[in] p base problem
///
/// The constructor is implemented so as to ensure that there is no violation
/// of the type system's constraints.
///
individual::individual(const problem &p)
  : ultra::individual(), genome_(p.env.slp.code_length, p.sset.categories()),
    active_crossover_type_(random::sup(NUM_CROSSOVERS))
{
  Expects(size());
  Expects(categories());

  const auto generate_index_line(
    [&](locus::index_t i, const auto &gen_f)
    {
      const symbol::category_t c_sup(categories());

      for (symbol::category_t c(0); c < c_sup; ++c)
      {
        gene &g(genome_(i, c));

        g.func = p.sset.roulette_function(c);

        const auto arity(g.func->arity());
        g.args.reserve(arity);

        const auto gen([&]() { return gen_f(c); });

        std::generate_n(std::back_inserter(g.args), arity, gen);
      }
    });

  generate_index_line(0, [&p](symbol::category_t c)
                         {
                           return p.sset.roulette_terminal(c);
                         });

  const locus::index_t i_sup(size());
  for (locus::index_t i(1); i < i_sup; ++i)
    generate_index_line(i, [&](symbol::category_t c)
                           {
                             return p.sset.roulette_terminal_and_param(i, c);
                           });

  Ensures(is_valid());
}

///
/// Creates a new individual containing genes from `gv`.
///
/// \param[in] gv vector of genes
///
/// This is useful for debugging purpose (i.e. setup *ad-hoc* individuals).
///
individual::individual(const std::vector<gene> &gv)
  : ultra::individual(),
    genome_(gv.size(),
            std::max_element(std::begin(gv), std::end(gv),
                             [](const gene &g1, const gene &g2)
                             {
                               return g1.category() < g2.category();
                             })->category() + 1),
    active_crossover_type_(random::sup(NUM_CROSSOVERS))
{
  locus::index_t i(0);

  for (const auto &g : gv)
    genome_(i++, g.category()) = g;

  Ensures(is_valid());
}


///
/// \return the total number of categories the individual is using
///
symbol::category_t individual::categories() const
{
  return static_cast<symbol::category_t>(genome_.cols());
}

///
/// \return `true` if the individual isn't initialized
///
bool individual::empty() const
{
  return size() == 0;
}

///
/// \return the total size of the individual (effective size + introns)
///
/// \remark
/// Size is constant for any individual (it's chosen at initialization time).
///
/// \see eff_size()
///
locus::index_t individual::size() const
{
  return static_cast<locus::index_t>(genome_.rows());
}

///
/// \param[in] l locus of a `gene`
/// \return      the `l`-th gene of `this` individual
///
const gene &individual::operator[](locus l) const
{
  return genome_(l);
}

///
/// \param[in] rhs second term of comparison
/// \return        `true` if the two individuals are equal (symbol by symbol,
///                including introns)
///
/// \note
/// Age is not checked.
///
bool individual::operator==(const individual &rhs) const
{
  const bool eq(genome_ == rhs.genome_);

  //assert(!eq
  //        || signature_.empty() != x.signature_.empty()
  //        || signature_ == x.signature_);

  return eq;
}

///
/// \param[in] in input stream
/// \param[in] ss active symbol set
/// \return       `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful the current individual isn't
/// modified.
///
bool individual::load_impl(std::istream &in, const symbol_set &ss)
{
  unsigned rows, cols;
  if (!(in >> rows >> cols))
    return false;

  // The matrix class has a basic support for serialization but we cannot
  // take advantage of it here: the gene struct needs special management
  // (among other things it needs access to the symbol_set to decode the
  // symbols).
  decltype(genome_) genome(rows, cols);
  for (auto &g : genome)
  {
    symbol::opcode_t opcode;
    if (!(in >> opcode))
      return false;

    gene temp;

    temp.func = get_if<function>(ss.decode(opcode));
    if (!temp.func)
      return false;

    if (const auto arity = temp.func->arity())
    {
      temp.args.resize(arity);

      for (auto &arg : temp.args)
      {
        int d;
        if (!(in >> d))
          return false;

        value_t v;
        switch (d)
        {
        case d_address:
          if (int x; in >> x)
            v = param_address(x);
          break;

        case d_double:
          if (double x; load_float_from_stream(in, &x))
            v = x;
          break;

        case d_int:
          if (int x; in >> x)
            v = x;
          break;

        case d_nullary:
          if (symbol::opcode_t x; in >> x)
            if (const auto *n = get_if<nullary>(ss.decode(x)))
              v = n;
          break;

        case d_string:
          if (std::string s; in >> s)
            v = s;
          break;

        case d_void:
          break;
        }

        if (!has_value(v))
          return false;

        arg = v;
      }
    }

    g = temp;
  }

  genome_ = genome;

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if the object has been saved correctly
///
bool individual::save_impl(std::ostream &out) const
{
  out << genome_.rows() << ' ' << genome_.cols() << '\n';
  for (const auto &g : genome_)
  {
    out << g.func->opcode();

    for (const auto &a : g.args)
    {
      out << a.index() << ' ';

      switch (a.index())
      {
      case d_address:
        out << as_integer(std::get<D_ADDRESS>(a));
        break;
      case d_double:
        save_float_to_stream(out, std::get<D_DOUBLE>(a));
        break;
      case d_int:
        out << std::get<D_INT>(a);
        break;
      case d_nullary:
        out << std::get<const D_NULLARY *>(a)->opcode();
        break;
      case d_string:
        out << std::get<D_STRING>(a);
        break;
      case d_void:
        break;
      }
    }

    out << '\n';
  }

  return out.good();
}

///
/// \return `true` if the individual passes the internal consistency check
///
bool individual::is_valid() const
{
  if (empty())
  {
    if (!genome_.empty())
    {
      ultraERROR << "Inconsistent internal status for empty individual";
      return false;
    }

    if (!signature_.empty())
    {
      ultraERROR << "Empty individual and non-empty signature";
      return false;
    }

    return true;
  }

  for (locus::index_t i(0); i < size(); ++i)
    for (symbol::category_t c(0); c < categories(); ++c)
    {
      const locus l(i, c);

      if (!genome_(l).func)
      {
        ultraERROR << "Empty symbol pointer at locus " << l;
        return false;
      }

      if (!genome_(l).is_valid())
      {
        ultraERROR << "Arity and actual arguments don't match";
        return false;
      }
    }

  // Type checking.
  for (locus::index_t i(0); i < size(); ++i)
    for (symbol::category_t c(0); c < categories(); ++c)
    {
      const locus l(i, c);

      if (genome_(l).category() != c)
      {
        ultraERROR << "Wrong category: " << l << genome_(l).func->name()
                   << " -> " << genome_(l).category()
                   << " should be " << c;
        return false;
      }
    }

  //if (categories() == 1 && active_symbols() > size())
  //{
  //  ultraERROR << "`active_symbols()` cannot be greater than `size()` "
  //               "in single-category individuals";
  //  return false;
  //}
  //
  //return signature_.empty() || signature_ == hash();

  return true;
}

}  // namespace ultra::gp
