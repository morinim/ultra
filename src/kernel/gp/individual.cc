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

#include "kernel/gp/individual.h"
#include "kernel/nullary.h"
#include "kernel/gp/src/variable.h"

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
individual::individual(const problem &p) : genome_(p.params.slp.code_length,
                                                   p.sset.categories())
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

  signature_ = hash();
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
  : genome_(gv.size(),
            std::ranges::max(gv,
                             [](const gene &g1, const gene &g2)
                             {
                               return g1.category() < g2.category();
                             }).category() + 1)
{
  locus::index_t i(0);

  for (const auto &g : gv)
    genome_(i++, g.category()) = g;

  signature_ = hash();
  Ensures(is_valid());
}

///
/// \return the total number of categories the individual is using
///
symbol::category_t individual::categories() const noexcept
{
  return static_cast<symbol::category_t>(genome_.cols());
}

///
/// \return `true` if the individual isn't initialized
///
bool individual::empty() const noexcept
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
locus::index_t individual::size() const noexcept
{
  return static_cast<locus::index_t>(genome_.rows());
}

///
/// \return a range to iterate through exons (active genes).
///
exon_view individual::exons()
{
  return exon_view(*this);
}

///
/// \return a const range to iterate through exons (active genes).
///
const_exon_view individual::cexons() const
{
  return const_exon_view(*this);
}

///
/// Serialises an individual (or subtree) into a byte stream.
///
/// \param[in]  l    root locus to be serialised
/// \param[out] sink destination sink receiving the serialised bytes
///
/// This function walks the structure rooted at `l` and emits a *canonical*
/// sequence of bytes representing its semantic content. The resulting byte
/// stream is independent of memory addresses and implementation details; it's
/// stable across executions and platforms.
///
/// ### Design goals
/// - **Determinism**. Identical individuals always produce identical byte
///   streams.
/// - **Completeness**. All information relevant to semantics and identity is
///   included.
/// - **Stability**. Changes in memory layout do not affect the output.
/// - **Composability**. Complex structures are packed by recursively packing
///   their components.
///
/// ### Notes
/// - `pack` performs no hashing by itself; it only defines *what* bytes are
///   emitted and in which order;
/// - the choice of hash function or storage strategy is delegated entirely
///   to the sink;
/// - the byte stream produced by `pack` is suitable for incremental
///   processing.
///
/// \see murmurhash3_sink
///
void individual::pack(const locus &l, hash_sink &sink) const
{
  const auto pack_span([&](const std::span<const std::byte> bytes)
  {
    sink.write(bytes);
  });

  const auto pack_value([&](const auto &v)
  {
    pack_span(bytes_view(v));
  });

  const auto pack_opcode([&](symbol::opcode_t opcode)
  {
    Expects(std::in_range<std::uint16_t>(opcode));

    // Although 16 bit are enough to contain opcodes and parameters, they are
    // usually stored in unsigned variables (i.e. 32 or 64 bit) for performance
    // reasons.
    // Anyway before hashing opcodes/parameters we convert them to 16 bit types
    // to avoid hashing more than necessary.
    // This also distinguish between an opcode and an integer value (the former
    // is treated as a 16bit number the latter as something bigger).
    const auto opcode16(static_cast<std::uint16_t>(opcode));
    pack_value(opcode16);
  });

  const gene &g(genome_(l));
  pack_opcode(g.func->opcode());

  for (std::size_t i(0); i < g.args.size(); ++i)
  {
    const auto &a(g.args[i]);

    std::visit([&](const auto &v)
    {
      using T = std::decay_t<decltype(v)>;

      if constexpr (std::same_as<T, D_ADDRESS>)
        pack(g.locus_of_argument(i), sink);
      else if constexpr (std::same_as<T, D_INT> || std::same_as<T, D_DOUBLE>
                         || std::same_as<T, D_STRING>)
        pack_value(v);
      else if constexpr (std::same_as<T, const D_NULLARY *>
                         || std::same_as<T, const D_VARIABLE *>)
        pack_opcode(v->opcode());
      else if constexpr (std::same_as<T, D_IVECTOR>)
        for (const auto &elem : v)
          pack_value(elem);
      else if constexpr (std::same_as<T, D_VOID>)
        /* nothing */;
      else
        static_assert(
          false, "Non-exhaustive visitor inside `gp::individual::pack`");
    }, a);
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
  if (!size())
    return hash_t();

  // From an individual to a packed byte stream...
  hash_sink sink;

  pack(start(), sink);
  return sink.final();
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
locus individual::start() const noexcept
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
bool individual::operator==(const individual &rhs) const noexcept
{
  const bool eq(genome_ == rhs.genome_);

  assert(!eq
         || signature().empty() != rhs.signature().empty()
         || signature() == rhs.signature());

  return eq;
}

///
/// Calculates the Hamming distance between two individuals.
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

  return hamming_distance(lhs, rhs);
}

///
/// Number of active functions.
///
/// \return number of active functions
///
/// \see
/// size()
///
/// When `category() > 1`, `active_slots()` can be greater than `size()`. For
/// instance consider the following individual:
///
///     [0,0] function returning a number
///     [1,0] function returning a number
///     [1,1] function returning a string
///     [2,1] function returning a string
///     [3,0] function [2,1] [1,1] [1,0] [0,0]
///
/// `size() == 4` (four slots / rows) and `active_slots() == 5`.
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
/// Extracts the decision vector associated with an individual.
///
/// \return a valid decision vector
///
/// The returned `decision_vector` provides a flat, optimiser-friendly view of
/// all tunable scalar parameters (currently `D_DOUBLE` and `D_INT`) appearing
/// in the active portion of the genome.
///
/// \note
/// Only parameters in active genes (exons) are included. Inactive code is
/// ignored.
///
/// \note
/// Integer parameters are converted to `double` for optimisation. The original
/// type information is preserved in `decision_vector::coordinate::kind` and
/// restored when applying the vector.
///
/// \note
/// The ordering of parameters is deterministic for a fixed individual and
/// reflects the traversal order of its active genes. The returned vector is
/// therefore compatible with `apply_decision_vector()` as long as the
/// structure of the individual is unchanged.
///
decision_vector extract_decision_vector(const individual &ind)
{
  decision_vector ret;
  using pk = dv_param_kind;

  const auto exons(ind.cexons());
  for (auto it(exons.begin()); it != exons.end(); ++it)
  {
    const auto &args(it->args);

    for (std::size_t j(0); j < args.size(); ++j)
      if (const auto *d = std::get_if<ultra::D_DOUBLE>(&args[j]))
      {
        ret.values.push_back(*d);
        ret.coords.push_back({{it.locus(), j}, pk::real});
      }
      else if (const auto *n = std::get_if<ultra::D_INT>(&args[j]))
      {
        ret.values.push_back(static_cast<double>(*n));
        ret.coords.push_back({{it.locus(), j}, pk::integer});
      }
  }

  Ensures(ret.is_valid());
  return ret;
}

///
/// Applies a decision vector to this individual.
///
/// \param[in] v decision vector previously extracted from a compatible
///              individual
///
/// \pre
/// - `v.is_valid()`;
/// - `v` must have been obtained from an individual with the same structure
///   (same active exons, loci, and argument layout). Applying a decision
///   vector to a structurally different individual results in undefined
///   behaviour.
///
/// Each entry of the decision vector is written back to the corresponding
/// argument in the genome, as identified by `decision_vector::coordinate`.
/// Real-valued parameters are copied directly, while integer parameters are
/// reconstructed by rounding the provided value to the nearest integer
/// (`std::lround`).
///
void individual::apply_decision_vector(const decision_vector &v)
{
  Expects(v.is_valid());

  using pk = dv_param_kind;

  for (std::size_t i(0); i < v.size(); ++i)
  {
    const auto &coord(v.coords[i].coord);
    const auto value(v.values[i]);

    if (auto &arg(genome_(coord.loc).args[coord.arg_index]);
        v.coords[i].kind == pk::integer)
      arg = static_cast<ultra::D_INT>(std::lround(value));
    else
      arg = static_cast<ultra::D_DOUBLE>(value);
  }

  signature_ = hash();
  Ensures(is_valid());
}

namespace internal
{

///
/// Internal implementation of GP crossover.
///
/// `crossover_engine` centralises all privileged genome rewriting required by
/// recombination. The public `crossover(...)` function remains the user-facing
/// API, while this helper dispatches to the selected elementary crossover
/// operator and finalises offspring metadata.
///
/// Keeping the implementation here avoids exposing multiple helper functions
/// as friends of `gp::individual`.
///
class crossover_engine
{
public:
  [[nodiscard]] individual operator()(const problem &,
                                      const individual &lhs,
                                      const individual &rhs) const;

private:
  struct context
  {
    // const problem &prob;
    const individual &from;
    individual to;
  };

  void copy_gene(context &, const locus &) const;
  void copy_range(context &, std::size_t, std::size_t) const;

  void finalise(context &) const;
  void run(context &) const;

  void one_point(context &) const;
  void two_points(context &) const;
  void tree(context &) const;
  void uniform(context &) const;
};

/// Copies a single locus from the donor into the recipient.
void crossover_engine::copy_gene(context &ctx, const locus &l) const
{
  ctx.to.genome_(l) = ctx.from[l];
}

/// Copies the half-open range `[first, last_exclusive)` from the donor into
/// the corresponding loci of the recipient.
void crossover_engine::copy_range(context &ctx, std::size_t first,
                                  std::size_t last_exclusive) const
{
  std::copy(std::next(ctx.from.begin(), first),
            std::next(ctx.from.begin(), last_exclusive),
            std::next(ctx.to.genome_.begin(), first));
}

/// One-point crossover.
///
/// The offspring is initialised as a copy of one parent. A crossover point is
/// then selected, and all loci in the suffix `[cut, end)` are copied from the
/// donor parent.
void crossover_engine::one_point(context &ctx) const
{
  const auto genes(std::ranges::distance(ctx.from));
  Expects(genes > 1);

  const auto cut(random::sup(genes - 1));
  copy_range(ctx, cut, genes);
}

/// Two-points crossover.
///
/// The offspring is initialised as a copy of one parent. Two crossover points
/// are selected, and the half-open interval `[cut1, cut2)` is copied from the
/// donor parent.
void crossover_engine::two_points(context &ctx) const
{
  const auto genes(std::ranges::distance(ctx.from));
  Expects(genes > 1);

  const auto cut1(random::sup(genes - 1));
  const auto cut2(random::between(cut1 + 1, genes));
  copy_range(ctx, cut1, cut2);
}

/// Uniform crossover.
///
/// The offspring is initialised as a copy of one parent. Then, for each locus,
/// the donor gene replaces the current one with probability 0.5.
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
void crossover_engine::uniform(context &ctx) const
{
  // NOTE: we are intentionally using `std::transform` instead of
  // `std::ranges::transform`. As of Clang 18.1.3, using ranges here
  // triggers a known Internal Compiler Error (ICE) in the frontend:
  // "error: cannot compile this l-value expression yet".
  // Do not refactor to ranges until CI confirms Clang stability for
  // nested lambda expressions in this context.
  std::transform(ctx.from.begin(), ctx.from.end(), ctx.to.genome_.begin(),
                 ctx.to.genome_.begin(),
                 [](const auto &g1, const auto &g2)
                 { return random::boolean() ? g1 : g2; });
}

/// Tree crossover.
///
/// A random active locus is selected in the donor, and the dependency-closed
/// subtree rooted at that locus is copied into the offspring at the same loci.
/// This is typically less disruptive than segment-based crossover because an
/// entire functional subexpression is preserved.
void crossover_engine::tree(context &ctx) const
{
  // FIXME: reverted to a standard lambda capture.
  // Although fixed in Clang 21, Clang 18.x (current CI/LTS baseline)
  // suffers from an Internal Compiler Error (ICE) when combining deducing
  // `this` with nested l-value member access:
  // "error: cannot compile this l-value expression yet"
  auto crossover_ = [&](const locus &l, const auto &self) -> void
  {
    copy_gene(ctx, l);

    for (const auto &al : ctx.from[l].args)
      if (std::holds_alternative<D_ADDRESS>(al))
        self(ctx.from[l].locus_of_argument(al), self);
  };

  crossover_(random_locus(ctx.from), crossover_);
}

/// Dispatches to the crossover operator selected by the donor parent.
void crossover_engine::run(context &ctx) const
{
  switch (ctx.from.active_crossover_type_)
  {
  case individual::crossover_t::one_point:
    one_point(ctx);
    break;

  case individual::crossover_t::two_points:
    two_points(ctx);
    break;

  case individual::crossover_t::uniform:
    uniform(ctx);
    break;

  default:
    tree(ctx);
  }
}

/// Restores offspring metadata after recombination.
///
/// In particular, this propagates the donor's active crossover type, updates
/// age information, and recomputes the structural signature.
void crossover_engine::finalise(context &ctx) const
{
  ctx.to.active_crossover_type_ = ctx.from.active_crossover_type_;
  ctx.to.set_if_older_age(ctx.from.age());
  ctx.to.signature_ = ctx.to.hash();
}

individual crossover_engine::operator()(const problem &,
                                        const individual &lhs,
                                        const individual &rhs) const
{
  Expects(lhs.size() == rhs.size());
  Expects(std::ranges::distance(lhs) == std::ranges::distance(rhs));

  const bool b(random::boolean());
  const auto &from(b ? rhs : lhs);
  auto to(b ? lhs : rhs);

  context ctx{from, std::move(to)};

  run(ctx);
  finalise(ctx);

  Ensures(ctx.to.is_valid());
  return std::move(ctx.to);
}

}  // namespace internal

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
/// \note
/// Parents must have the same size.
///
/// \remark
/// The adaptation of the crossover parameter happens before the offspring is
/// assigned fitness. As a consequence, choosing a good operator does not
/// directly increase the individual's current fitness, but may improve its
/// reproductive performance over time.
///
/// \see
/// - https://github.com/morinim/ultra/wiki/bibliography#1
/// - https://github.com/morinim/ultra/wiki/bibliography#2
///
/// \relates gp::individual
///
individual crossover(const problem &p,
                     const individual &lhs, const individual &rhs)
{
  return internal::crossover_engine{}(p, lhs, rhs);
}

///
/// A new individual is created mutating `this`.
///
/// \param[in] prb the current problem
/// \return        number of mutations performed
///
/// \note
/// External parameters: `evolution.p_mutation`
///
unsigned individual::mutation(const problem &prb)
{
  const double pgm(prb.params.evolution.p_mutation);
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
        const auto c(i->func->param_category(pos));
        i->args[pos] = prb.sset.roulette_terminal(idx, c);
      }

      ++n;
    }

  if (n)
    signature_ = hash();

  Ensures(is_valid());
  return n;
}

///
/// \return the active crossover type for `this` individual
///
individual::crossover_t individual::active_crossover_type() const noexcept
{
  return active_crossover_type_;
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
        if (value_t v; !ultra::load(in, ss, v) || !has_value(v))
          return false;
        else
          arg = v;
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
      out << ' ';

      if (!ultra::save(out, a))
        return false;
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

    if (!signature().empty())
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

  if (signature() != hash())
  {
    ultraERROR << "Actual signature ("
               << signature() << ") doesn't match the individual` ("
               << hash() << ')';
    return false;
  }

  return true;
}

}  // namespace ultra::gp

// ********************************************************************
// * PRINTING RELATED FUNCTIONS                                       *
// ********************************************************************
namespace
{

[[nodiscard]] std::string format_call(std::string_view fmt,
                                      std::span<const std::string> args)
{
  const auto is_digit([](char c)
  {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
  });

  std::string out;
  out.reserve(fmt.size() + args.size() * 8);

  for (std::size_t i(0); i < fmt.size(); ++i)
    switch (fmt[i])
    {
    case '{':
      if (i + 1 < fmt.size() && fmt[i + 1] == '{')
      {
        out.push_back('{');
        ++i;
        break;
      }

      if (++i >= fmt.size())
        throw std::format_error("Unmatched '{' in GP format string");

      if (!is_digit(fmt[i]))
        throw std::format_error("Expected argument index after '{'");

      {
        std::size_t index(0);
        while (i < fmt.size() && is_digit(fmt[i]))
          index = index * 10 + static_cast<std::size_t>(fmt[i++] - '0');

        if (i >= fmt.size() || fmt[i] != '}')
          throw std::format_error("Missing closing '}' in GP format string");

        if (index >= args.size())
          throw std::format_error("Argument index out of range in GP format "
                                  "string");

        out += args[index];
      }
      break;

    case '}':
      if (i + 1 < fmt.size() && fmt[i + 1] == '}')
      {
        out.push_back('}');
        ++i;
        break;
      }

      throw std::format_error("Unmatched '}' in GP format string");

    default:
      out.push_back(fmt[i]);
    }

  return out;
}

void print_locus(std::ostream &s, const ultra::gp::individual &prg,
                 const ultra::locus &l)
{
  Expects(prg.categories());

  constexpr auto decimal_width([](std::size_t n)
  {
    int w(1);
    for (; n >= 10; ++w)
      n /= 10;

    return w;
  });

  const auto w1(decimal_width(prg.size() - 1));
  const auto w2(decimal_width(prg.categories() - 1));

  ultra::SAVE_FLAGS(s);
  s << '[' << std::setfill('0') << std::setw(w1) << l.index;

  if (prg.categories() > 1)
    s << ',' << std::setw(w2) << l.category;

  s << ']';
}

void print_arg(std::ostream &s, ultra::symbol::format fmt,
               const ultra::gp::individual &prg, const ultra::gene &g,
               std::size_t idx)
{
  const auto a(g.args[idx]);

  switch (a.index())
  {
  case ultra::d_address:
    print_locus(s, prg, g.locus_of_argument(idx));
    break;
  case ultra::d_nullary:
    s << std::get<const ultra::D_NULLARY *>(a)->to_string(fmt);
    break;
  default:
    s << a;
  }
}

void print_gene(std::ostream &s, const ultra::gp::individual &prg,
                const ultra::gene &g)
{
  if (g.func)
  {
    s << ' ' << g.func->name();

    for (std::size_t j(0); j < g.args.size(); ++j)
    {
      s << ' ';
      print_arg(s, ultra::symbol::c_format, prg, g, j);
    }
  }
}

void print_language(std::ostream &s, ultra::symbol::format fmt,
                    const ultra::gp::individual &prg)
{
  // NOTE: keep recursion in C++20-compatible form (explicit `self` parameter
  // passed as first argument). GitHub's current clang toolchain used by CI
  // doesn't fully support "deducing this" lambdas yet.
  const auto language_ =
    [&](auto &&self, const ultra::gene &g) -> std::string
    {
      std::vector<std::string> args;
      args.reserve(g.func->arity());

      for (std::size_t i(0); i < g.func->arity(); ++i)
        if (g.args[i].index() != ultra::d_address)
        {
          std::ostringstream ss;
          print_arg(ss, fmt, prg, g, i);
          args.push_back(ss.str());
        }
        else
          args.push_back(self(self, prg[g.locus_of_argument(i)]));

      return format_call(g.func->to_string(fmt), args);
    };

  std::string out(language_(language_, prg[prg.start()]));
  if (out.length() > 2 && out.front() == '(' && out.back() == ')')
    out = out.substr(1, out.length() - 2);

  s << out;
}

void print_in_line(std::ostream &s, const ultra::gp::individual &prg)
{
  // NOTE: avoid "deducing this" here for compatibility with CI clang.
  const auto in_line_ = [&](auto &&self, ultra::locus l) -> void
  {
    const auto &g(prg[l]);

    if (l != prg.start())
      s << ' ';
    s << g.func->name();

    for (const auto &a : g.args)
      if (a.index() != ultra::d_address)
        s << ' ' << a;
      else
        self(self, g.locus_of_argument(a));
  };

  in_line_(in_line_, prg.start());
}

void print_dump(std::ostream &s, const ultra::gp::individual &prg)
{
  ultra::SAVE_FLAGS(s);

  const auto size(prg.size());
  const auto categories(prg.categories());

  for (ultra::locus::index_t i(0); i < size; ++i)
    for (ultra::symbol::category_t c(0); c < categories; ++c)
    {
      const ultra::locus l(i, c);
      print_locus(s, prg, l);
      print_gene(s, prg, prg[l]);

      s << '\n';
    }
}

void print_graphviz(std::ostream &s, const ultra::gp::individual &prg)
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
      case ultra::d_address:
        s << 'g' << std::get<ultra::D_ADDRESS>(i->args[j]) << '_'
          << i->func->param_category(j) << arg_ord_attr;
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

        if (index == ultra::d_nullary) s << '"';
        s << i->args[j];
        if (index == ultra::d_nullary) s << '"';

        s << "];\n";
        break;
      }
      }
    }
  }

  s << '}';
}

void print_list(std::ostream &s, const ultra::gp::individual &prg)
{
  ultra::SAVE_FLAGS(s);

  const auto exr(prg.cexons());
  for (auto i(exr.begin()); i != exr.end(); ++i)
  {
    print_locus(s, prg, i.locus());
    print_gene(s, prg, *i);

    s << '\n';
  }
}

void print_tree(std::ostream &s, const ultra::gp::individual &prg)
{
  // NOTE: avoid "deducing this" here for compatibility with CI clang.
  const auto tree_ = [&](auto &&self, const ultra::gene &curr,
                         unsigned indent) -> void
  {
    s << std::string(indent, ' ') << curr.func->name() << '\n';

    indent += 2;

    for (std::size_t i(0); i < curr.args.size(); ++i)
      switch (curr.args[i].index())
      {
      case ultra::d_address:
        self(self, prg[curr.locus_of_argument(i)], indent);
        break;

      default:
        s << std::string(indent, ' ');
        print_arg(s, ultra::symbol::c_format, prg, curr, i);
        s << '\n';
        break;
      }
  };

  tree_(tree_, prg[prg.start()], 0);
}

}  // namespace

namespace ultra::gp
{

void individual::print_impl(std::ostream &s, out::print_format_t format) const
{
  switch (format)
  {
  case out::dump_f:
    print_dump(s, *this);
    return;

  case out::in_line_f:
    print_in_line(s, *this);
    return;

  case out::graphviz_f:
    print_graphviz(s, *this);
    return;

  case out::list_f:
    print_list(s, *this);
    return;

  case out::tree_f:
    print_tree(s, *this);
    return;

  default:
    assert(format >= out::language_f);
    print_language(s, symbol::format(format - out::language_f), *this);
  }
}

}  // namespace ultra::gp
