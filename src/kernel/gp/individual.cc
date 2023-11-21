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

#include <span>

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

individual::exons_range<individual::exon_iterator> individual::exons()
{
  return {exon_iterator(*this), exon_iterator()};
}

individual::exons_range<individual::const_exon_iterator>
individual::exons() const
{
  return {const_exon_iterator(*this), const_exon_iterator()};
}

namespace
{
template <class T>
std::span<const std::byte, sizeof(T)> bytes_view(const T &t)
{
  return std::span<const std::byte, sizeof(T)>{
    reinterpret_cast<const std::byte *>(std::addressof(t)), sizeof(T)};
}

std::span<const std::byte> bytes_view(const std::string &s)
{
  return {reinterpret_cast<const std::byte *>(s.data()), s.length()};
}
}  // namespace

///
/// Maps syntactically distinct (but logically equivalent) individuals to the
/// same byte stream.
///
/// \param[in]  l locus in this individual
/// \param[out] p byte stream compacted version of the gene sequence
///               starting at locus `l`
///
/// Useful for individual comparison / information retrieval.
///
void individual::pack(const locus &l, std::vector<std::byte> *p) const
{
  const auto pack_span([&p](const std::span<const std::byte> data)
  {
    p->insert(p->end(), data.begin(), data.end());
  });

  const auto pack_value([&](const auto &data)
  {
    pack_span(bytes_view(data));
  });

  const auto pack_opcode([&](const symbol::opcode_t &opcode)
  {
    Expects(opcode <= std::numeric_limits<symbol::opcode_t>::max());

    // Although 16 bit are enough to contain opcodes and parameters, they are
    // usually stored in unsigned variables (i.e. 32 or 64 bit) for performance
    // reasons.
    // Anyway before hashing opcodes/parameters we convert them to 16 bit types
    // to avoid hashing more than necessary.
    const auto opcode16(static_cast<std::uint16_t>(opcode));

    pack_value(opcode16);
  });

  const gene &g(genome_(l));
  pack_opcode(g.func->opcode());

  for (const auto &a : g.args)
    switch (a.index())
    {
    case d_address:
      pack(g.locus_of_argument(a), p);
      break;
    case d_double:
      pack_value(std::get<D_DOUBLE>(a));
      break;
    case d_int:
      pack_value(std::get<D_INT>(a));
      break;
    case d_nullary:
      pack_opcode(std::get<const D_NULLARY *>(a)->opcode());
      break;
    case d_string:
      pack_value(std::get<D_STRING>(a));
      break;
    case d_void:
      break;
    }
}

///
/// Converts the individual in a packed byte representation and performs the
/// hash algorithm on it.
///
/// \return the signature of this individual
///
hash_t individual::hash() const
{
  Expects(size());

  // From an individual to a packed byte stream...
  thread_local std::vector<std::byte> packed;

  packed.clear();
  pack(start(), &packed);

  return ultra::hash::hash128(packed.data(), packed.size());
}

///
/// Signature maps syntactically distinct (but logically equivalent)
/// individuals to the same value.
///
/// \return the signature of this individual.
///
/// In other words identical individuals at genotypic level have the same
/// signature; different individuals at the genotipic level may be mapped
/// to the same signature since real structure/computation is considered and
/// not the simple storage.
///
/// This is a very interesting  property, useful for individual comparison,
/// information retrieval, entropy calculation...
///
hash_t individual::signature() const
{
  if (signature_.empty())
    signature_ = hash();

  return signature_;
}

///
/// \param[in] l locus of a `gene`
/// \return      the `l`-th gene of `this` individual
///
const gene &individual::operator[](const locus &l) const
{
  return genome_(l);
}

///
/// \return the first gene of the individual (the first instruction of the
///         program)
///
locus individual::start() const
{
  return {size() - 1, symbol::default_category};
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

  assert(!eq
         || signature_.empty() != rhs.signature_.empty()
         || signature_ == rhs.signature_);

  return eq;
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs` (the number of different genes between individuals)
///
/// \relates gp::individual
///
unsigned distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.size() == rhs.size());
  Expects(lhs.categories() == rhs.categories());

  const locus::index_t i_sup(lhs.size());
  const symbol::category_t c_sup(lhs.categories());

  unsigned d(0);
  for (locus::index_t i(0); i < i_sup; ++i)
    for (symbol::category_t c(0); c < c_sup; ++c)
    {
      const locus l{i, c};
      if (lhs[l] != rhs[l])
        ++d;
    }

  return d;
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
      out << ' ' << a.index() << ' ';

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

namespace
{

std::ostream &print_arg(std::ostream &s, symbol::format fmt,
                        const individual &prg,
                        const gene &g, std::size_t idx)
{
  const auto a(g.args[idx]);

  switch (a.index())
  {
  case d_address:
    if (prg.categories() > 1)
      s << g.locus_of_argument(idx);
    else
      s << '[' << g.locus_of_argument(idx).index << ']';
    break;
  case d_nullary:
    s << std::get<const D_NULLARY *>(a)->to_string(fmt);
    break;
  default:
    s << a;
    break;
  }

  return s;
}

std::ostream &language(std::ostream &s, symbol::format fmt,
                       const individual &prg)
{
  std::function<std::string (const gene &)> language_;
  language_ = [&](const gene &g)
              {
                std::string ret(g.func->to_string(fmt));

                for (std::size_t i(0); i < g.func->arity(); ++i)
                {
                  const std::string from("{" + std::to_string(i) + "}");

                  if (g.args[i].index() != d_address)
                  {
                    std::stringstream ss;
                    print_arg(ss, fmt, prg, g, i);
                    ret = replace_all(ret, from, ss.str());
                  }
                  else
                    ret = replace_all(ret, from,
                                      language_(prg[g.locus_of_argument(i)]));
                }

                return ret;
              };

  std::string out(language_(prg[prg.start()]));
  if (out.length() > 2 && out.front() == '(' && out.back() == ')')
    out = out.substr(1, out.length() - 2);

  return s << out;
}

std::ostream &in_line(std::ostream &s, const individual &prg)
{
  std::function<void (locus)> in_line_;
  in_line_ = [&](locus l)
             {
               const gene &g(prg[l]);

               if (l != prg.start())
                 s << ' ';
               s << g.func->name();

               for (const auto &a : g.args)
                  if (a.index() != d_address)
                    s << ' ' << a;
                  else
                    in_line_(g.locus_of_argument(a));
             };

  in_line_(prg.start());
  return s;
}

std::ostream &dump(std::ostream &s, const individual &prg)
{
  SAVE_FLAGS(s);

  const auto size(prg.size());
  const auto categories(prg.categories());

  const auto w1(1 + static_cast<int>(std::log10(size - 1)));
  const auto w2(1 + static_cast<int>(std::log10(categories)));

  for (locus::index_t i(0); i < size; ++i)
    for (symbol::category_t c(0); c < categories; ++c)
    {
      const gene &g(prg[{i, c}]);

      s << '[' << std::setfill('0') << std::setw(w1) << i;

      if (categories > 1)
        s << ',' << std::setw(w2) << c;

      s  << "] " << g.func->name();

      for (std::size_t j(0); j < g.args.size(); ++j)
      {
        s << ' ';
        print_arg(s, symbol::c_format, prg, g, j);
      }

      s << '\n';
    }

  return s;
}

}  // namespace

///
/// \param[out] s   output stream
/// \param[in]  prg individual to be printed
/// \return         output stream including `prg`
///
/// \relates gp::individual
///
std::ostream &operator<<(std::ostream &s, const individual &prg)
{
  const auto format(out::print_format_flag(s));

  switch (format)
  {
  case out::dump_f:
    return dump(s, prg);

  case out::in_line_f:
    return in_line(s, prg);

/*
  case out::graphviz_f:
    graphviz(ind, s);
    return s;

  case out::list_f:
    return list(ind, s);

  case out::tree_f:
    return tree(ind, s);
*/
  default:
    assert(format >= out::language_f);
    return language(s, symbol::format(format - out::language_f), prg);
  }
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

  return signature_.empty() || signature_ == hash();
}

}  // namespace ultra::gp
