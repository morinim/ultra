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

#include <functional>
#include <span>

#include "kernel/gp/individual.h"
#include "kernel/nullary.h"
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
  : ultra::individual(), genome_(p.env.slp.code_length, p.sset.categories())
{
  Expects(size());
  Expects(categories());

  const locus::index_t i_sup(size());
  const symbol::category_t c_sup(categories());

  for (locus::index_t i(0); i < i_sup; ++i)
    for (symbol::category_t c(0); c < c_sup; ++c)
      if (p.sset.functions(c))
      {
        gene &g(genome_(i, c));

        g.func = p.sset.roulette_function(c);
        g.args.reserve(g.func->arity());

        std::ranges::transform(g.func->categories(),
                               std::back_inserter(g.args),
                               [&](auto arg_c)
                               {
                                 return p.sset.roulette_terminal(i, arg_c);
                               });
      }

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
            std::ranges::max(gv,
                             [](const gene &g1, const gene &g2)
                             {
                               return g1.category() < g2.category();
                             }).category() + 1)
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
  return genome_.empty();
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
/// \return a range to iterate through exons (active genes).
///
individual::exon_range individual::exons()
{
  return {exon_iterator(*this), exon_iterator()};
}

///
/// \return a const range to iterate through exons (active genes).
///
individual::const_exon_range individual::cexons() const
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

  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0u,
                            std::plus{}, std::not_equal_to{});
}

///
/// Number of active functions.
///
/// \return number of active functions
///
/// \see
/// size()
///
/// When `category() > 1`, active_functions() can be greater than size(). For
/// instance consider the following individual:
///
///     [0,0] function returning a number
///     [1,0] function returning a number
///     [1,1] function returning a string
///     [2,1] function returning a string
///     [3,0] function [2,1] [1,1] [1,0] [0,0]
///
/// `size() == 4` (four slots / rows) and `active_functions() == 5`.
///
/// \relates gp::individual
///
unsigned active_slots(const individual &prg)
{
  return std::ranges::distance(prg.cexons());
}

locus random_locus(const individual &prg)
{
  const auto exs(prg.cexons());

  std::vector<locus> loci;
  for (auto it(exs.begin()); it != exs.end(); ++it)
    loci.push_back(it.locus());

  return random::element(loci);
}

///
/// A Self-Adaptive Crossover operator.
///
/// \param[in] lhs first parent
/// \param[in] rhs second parent
/// \return        the result of the crossover (we only generate a single
///                offspring)
///
/// Well known elementary crossover operators traverse the problem domain in
/// different ways, exhibiting variable performances and specific problems.
/// An attempt to make the algorithm more robust is combining various search
/// strategies, encapsulated by the different elementary crossover operators
/// available, via self adaptation.
///
/// We associate to each individual the type of crossover used to create it
/// (initially this is set to a random type). This type is used afterwards to
/// determine which crossover to apply and allows the algorithm to adjust the
/// relative mixture of operators.
///
/// Here we briefly describe the elementary crossover operators that are
/// utilised:
///
/// **ONE POINT**
///
/// We randomly select a parent (between `from` and `to`) and a single locus
/// (common crossover point). The offspring is created with genes from the
/// chosen parent up to the crossover point and genes from the other parent
/// beyond that point.
/// One-point crossover is the oldest homologous crossover in tree-based GP.
///
/// **TREE**
///
/// Inserts a complete tree from one parent into the other.
/// The operation is less disruptive than other forms of crossover since
/// an entire tree is copied (not just a part).
///
/// **TWO POINTS**
///
/// We randomly select two loci (common crossover points). The offspring is
/// created with genes from the one parent before the first crossover point and
/// after the second crossover point; genes between crossover points are taken
/// from the other parent.
///
/// **UNIFORM CROSSOVER**
///
/// The i-th locus of the offspring has a 50% probability to be filled with
/// the i-th gene of `from` and 50% with i-th gene of `to`.
///
/// Uniform crossover, as the name suggests, is a GP operator inspired by the
/// GA operator of the same name. GA uniform crossover constructs offspring on
/// a bitwise basis, copying each allele from each parent with a 50%
/// probability. Thus the information at each gene location is equally likely
/// to have come from either parent and on average each parent donates 50%
/// of its genetic material. The whole operation, of course, relies on the
/// fact that all the chromosomes in the population are of the same structure
/// and the same length. GP uniform crossover begins with the observation that
/// many parse trees are at least partially structurally similar.
///
/// \note
/// Parents must have the same size.
///
/// \remark
/// What has to be noticed is that the adaption of the parameter happens before
/// the fitness is given to it. That means that getting a good parameter
/// doesn't rise the individual's fitness but only its performance over time.
///
/// \see
/// - https://github.com/morinim/ultra/wiki/bibliography#1
/// - https://github.com/morinim/ultra/wiki/bibliography#2
///
/// \relates
/// individual
///
individual crossover(const individual &lhs, const individual &rhs)
{
  Expects(lhs.size() == rhs.size());
  Expects(std::distance(lhs.begin(), lhs.end())
          == std::distance(rhs.begin(), rhs.end()));

  const bool b(random::boolean());
  const auto &from(b ? rhs : lhs);
  auto          to(b ? lhs : rhs);

  const auto genes(std::distance(from.begin(), from.end()));

  switch (from.active_crossover_type_)
  {
  case individual::crossover_t::one_point:
  {
    const auto cut(random::sup(genes - 1));

    std::copy(std::next(from.begin(), cut), from.end(),
              std::next(to.begin(), cut));
    break;
  }

  case individual::crossover_t::two_points:
  {
    const auto cut1(random::sup(genes - 1));
    const auto cut2(random::between(cut1 + 1, genes));

    std::copy(std::next(from.begin(), cut1), std::next(from.begin(), cut2),
              std::next(to.begin(), cut1));
    break;
  }

  case individual::crossover_t::uniform:
    std::transform(from.begin(), from.end(), to.begin(), to.begin(),
                   [](const auto &g1, const auto &g2)
                   { return random::boolean() ? g1 : g2; });
    break;

  default:  // Tree crossover
    {
      auto crossover_ = [&](const locus &l, const auto &lambda) -> void
      {
        to.genome_(l) = from[l];

        for (const auto &al : from[l].args)
          if (std::holds_alternative<D_ADDRESS>(al))
            lambda(from[l].locus_of_argument(al), lambda);
      };

      crossover_(random_locus(from), crossover_);
    }
    break;
  }

  to.active_crossover_type_ = from.active_crossover_type_;
  to.set_if_older_age(from.age());
  to.signature_.clear();

  Ensures(to.is_valid());
  return to;
}

///
/// A new individual is created mutating `this`.
///
/// \param[in] prb the current problem
/// \return        number of mutations performed
///
unsigned individual::mutation(const problem &prb)
{
  const double pgm(prb.env.evolution.p_mutation);
  Expects(0.0 <= pgm && pgm <= 1.0);

  unsigned n(0);

  const auto re(exons());
  for (auto i(re.begin()); i != re.end(); ++i)  // mutation affects only exons
    if (random::boolean(pgm))
    {
      const auto idx(i.locus().index);

      if (const auto pos(random::sup(i->args.size() + 1));
          pos == i->args.size())
      {
        gene g;
        g.func = prb.sset.roulette_function(i->category());
        g.args.reserve(g.func->arity());

        std::ranges::transform(
          g.func->categories(), std::back_inserter(g.args),
          [&](symbol::category_t c)
          {
            return prb.sset.roulette_terminal(idx, c);
          });

        *i = g;
      }
      else  // input parameter
      {
        const auto c(i->func->categories(pos));
        i->args[pos] = prb.sset.roulette_terminal(idx, c);
      }

      ++n;
    }

  if (n)
    signature_.clear();

  Ensures(is_valid());
  return n;
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

std::string print_locus(const individual &prg, const locus &l)
{
  const auto w1(1 + static_cast<int>(std::log10(prg.size() - 1)));
  const auto w2(1 + static_cast<int>(std::log10(prg.categories())));

  std::stringstream ss;
  ss << '[' << std::setfill('0') << std::setw(w1) << l.index;

  if (prg.categories() > 1)
    ss << ',' << std::setw(w2) << l.category;

  ss  << "]";

  return ss.str();
}

std::ostream &print_arg(std::ostream &s, symbol::format fmt,
                        const individual &prg,
                        const gene &g, std::size_t idx)
{
  const auto a(g.args[idx]);

  switch (a.index())
  {
  case d_address:
    s << print_locus(prg, g.locus_of_argument(idx));
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

std::ostream &print_gene(std::ostream &s, const individual &prg, const gene &g)
{
  if (g.func)
  {
    s << ' ' << g.func->name();

    for (std::size_t j(0); j < g.args.size(); ++j)
    {
      s << ' ';
      print_arg(s, symbol::c_format, prg, g, j);
    }
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

  for (locus::index_t i(0); i < size; ++i)
    for (symbol::category_t c(0); c < categories; ++c)
    {
      const locus l(i, c);
      s << print_locus(prg, l);

      print_gene(s, prg, prg[l]);

      s << '\n';
    }

  return s;
}

std::ostream &graphviz(std::ostream &s, const individual &prg)
{
  s << "graph\n{\n";

  const auto exr(prg.cexons());
  for (auto i(exr.begin()); i != exr.end(); ++i)
  {
    s << 'g' << i.locus().index << '_' << i.locus().category << " [label="
      << std::quoted(i->func->name()) << ", shape=box];\n";

    for (unsigned j(0); j < i->func->arity(); ++j)
    {
      s << 'g' << i.locus().index << '_' << i.locus().category << " -- ";

      const std::string arg_ord_attr(" [label="
                                     + std::to_string(j)
                                     + ", fontcolor=lightgray];\n");

      const auto index(i->args[j].index());
      switch (index)
      {
      case d_address:
        s << 'g' << std::get<D_ADDRESS>(i->args[j]) << '_'
          << i->func->categories(j) << arg_ord_attr;
        break;
      default:
      {
        const std::string arg_unique_id(
          "a"
          + std::to_string(i.locus().index) + "_"
          + std::to_string(i.locus().category)  + "_"
          + std::to_string(j));

        s << arg_unique_id << arg_ord_attr
          << arg_unique_id << " [label=";

        if (index == d_nullary) s << '"';
        s << i->args[j];
        if (index == d_nullary) s << '"';

        s << "];\n";
        break;
      }
      }
    }
  }

  s << '}';

  return s;
}

std::ostream &list(std::ostream &s, const individual &prg)
{
  SAVE_FLAGS(s);

  const auto exr(prg.cexons());
  for (auto i(exr.begin()); i != exr.end(); ++i)
  {
    s << print_locus(prg, i.locus());

    print_gene(s, prg, *i);

    s << '\n';
  }

  return s;
}

std::ostream &tree(std::ostream &s, const individual &prg)
{
  std::function<void (const gene &, unsigned)> tree_;
  tree_ = [&](const gene &curr, unsigned indent)
          {
            s << std::string(indent, ' ') << curr.func->name() << '\n';

            indent += 2;

            for (std::size_t i(0); i < curr.args.size(); ++i)
              switch (curr.args[i].index())
              {
              case d_address:
                tree_(prg[curr.locus_of_argument(i)], indent);
                break;
              default:
                s << std::string(indent, ' ');
                print_arg(s, symbol::c_format, prg, curr, i);
                s << '\n';
                break;
              }
          };

  tree_(prg[prg.start()], 0);
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

  case out::graphviz_f:
    return graphviz(s, prg);

  case out::list_f:
    return list(s, prg);

  case out::tree_f:
    return tree(s, prg);

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

  if (!genome_(start()).func)
  {
    ultraERROR << "Empty function pointer at start (" << start() << ")";
    return false;
  }

  // Check function and arguments consistency (both number of arguments and
  // category).
  for (locus::index_t i(0); i < size(); ++i)
    for (symbol::category_t c(0); c < categories(); ++c)
    {
      const locus l(i, c);
      const gene &g(genome_(l));

      if (!g.is_valid())
      {
        ultraERROR << "Arity and actual arguments don't match";
        return false;
      }

      if (const auto *func = g.func)
      {
        if (func->category() != c)
        {
          ultraERROR << "Wrong category: " << l << ' ' << func->name()
                     << " -> " << g.category() << " should be " << c;
          return false;
        }

        for (const auto &a : g.args)
          switch (a.index())
          {
          case d_address:
            if (const auto al(g.locus_of_argument(a)); al.index >= i)
            {
              ultraERROR << "Argument `" << get_index(a, g.args)
                         << "` (`" << a << "`) of function `" << l << ' '
                         << func->name() << "` should be < `" << i << "`)";
              return false;
            }
            else if (!genome_(al).func)
            {
              ultraERROR << "Argument `" << get_index(a, g.args)
                         << "` of function `" << l << ' ' << func->name()
                         << "` is the address `" << al << "` of an empty gene";
              return false;
            }
            break;

          case d_nullary:
            if (const auto *n(get_if_nullary(a)); n->category() != c)
            {
              ultraERROR << "Argument `" << get_index(a, g.args)
                         << "` of function `" << l << ' ' << func->name()
                         << "` is the nullary `" << a << " -> " << n->category()
                         << "` but category should be `" << c << '`';
              return false;
            }
            break;
          }
      }
    }

  if (categories() == 1 && active_slots(*this) > size())
  {
    ultraERROR << "`active_functions()` (== " << active_slots(*this)
               << ") cannot be greater than `size()` (" << size()
               << ") in single-category individuals";
    return false;
  }

  return signature_.empty() || signature_ == hash();
}

}  // namespace ultra::gp
