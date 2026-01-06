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

#include "kernel/ga/individual.h"
#include "kernel/hash_t.h"
#include "kernel/random.h"

#include "utility/log.h"
#include "utility/misc.h"

#include <algorithm>

namespace ultra::ga
{
///
/// Constructs a new, random GA individual.
///
/// \param[in] p current problem
///
/// The process that generates the initial, random expressions has to be
/// implemented so as to ensure that they don't violate the type system's
/// constraints.
///
individual::individual(const ultra::problem &p) : genome_(p.sset.categories())
{
  Expects(parameters());

  std::ranges::generate(
    genome_,
    [&, n = 0]() mutable
    {
      return std::get<D_INT>(p.sset.roulette_terminal(n++));
    });

  signature_ = hash();
  Ensures(is_valid());
}

///
/// \return a const iterator pointing to the first gene
///
individual::const_iterator individual::begin() const noexcept
{
  return genome_.begin();
}

///
/// \return a const iterator pointing to a end-of-genome sentry
///
individual::const_iterator individual::end() const noexcept
{
  return genome_.end();
}

///
/// Returns the value of the gene at specified location.
///
/// \param[in] i position of the gene to return
/// \return      the requested gene
///
/// \warning
/// Accessing a nonexistent element through this operator is undefined
/// behavior.
///
const individual::value_type &individual::operator[](std::size_t i) const
{
  Expects(i < parameters());
  return genome_[i];
}

///
/// This is sweet "syntactic sugar" to manage individuals as integer value
/// vectors.
///
/// \return a vector of integer values
///
individual::operator std::vector<individual::value_type>() const noexcept
{
  return genome_;
}

///
/// Sets up the individual with values from a vector.
///
/// \param[in] v input vector (a point in a multidimensional space)
/// \return      a reference to `*this`
///
individual &individual::operator=(const std::vector<individual::value_type> &v)
{
  Expects(empty() || v.size() == size());

  genome_ = v;
  signature_ = hash();

  Ensures(is_valid());
  return *this;
}

///
/// Mutates the current individual.
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

  const auto ps(parameters());
  for (symbol::category_t c(0); c < ps; ++c)
    if (random::boolean(pgm))
      if (const auto g(std::get<D_INT>(prb.sset.roulette_terminal(c)));
          g != genome_[c])
      {
        ++n;
        genome_[c] = g;
      }

  if (n)
    signature_ = hash();

  Ensures(is_valid());
  return n;
}

///
/// \return `true` if the individual is empty, `false` otherwise
///
bool individual::empty() const noexcept
{
  return genome_.empty();
}

///
/// \return the number of parameters stored in the individual
///
std::size_t individual::parameters() const noexcept
{
  return size();
}

///
/// \return the number of parameters stored in the individual
///
std::size_t individual::size() const noexcept
{
  return genome_.size();
}

///
/// The signature (hash value) of this individual.
///
/// \return the signature of this individual
///
/// Identical individuals, at genotypic level, have same signature. The
/// signature is calculated just at the first call and then stored inside the
/// individual.
///
/// \remark
/// Concurrent calls to `signature()` on the same instance are safe, provided
/// the instance is not mutated concurrently.
///
hash_t individual::signature() const noexcept
{
  return signature_;
}

///
/// Calculates the Hamming distance between two individuals.
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparsion
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs` (the number of different genes)
///
/// \relates ga::individual
///
unsigned distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  return hamming_distance(lhs, rhs);
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if the two individuals are equal
///
/// \note
/// Age isn't checked.
///
/// \relates ga::individual
///
bool operator==(const individual &lhs, const individual &rhs)
{
  return std::ranges::equal(lhs, rhs);
}

///
/// Inserts into the output stream the graph representation of the individual.
///
/// \param[out] s  output stream
/// \param[in]  ga data to be printed
///
/// \note
/// The format used to describe the graph is the dot language
/// (https://www.graphviz.org/).
///
/// \relates ga::individual
///
std::ostream &graphviz(std::ostream &s, const individual &ga)
{
  s << "graph {";

  for (const auto &g : ga)
    s << "g [label=" << g << ", shape=circle];";

  s << '}';

  return s;
}

///
/// Prints the genes of the individual.
///
/// \param[out] s  output stream
/// \param[in]  ga data to be printed
/// \return        a reference to the output stream
///
/// \relates ga::individual
///
std::ostream &in_line(std::ostream &s, const individual &ga)
{
  if (!ga.empty())
  {
    s << *ga.begin();

    for (auto it(std::next(ga.begin())); it != ga.end(); ++it)
      s << " " << *it;
  }

  return s;
}

///
/// Two points crossover.
///
/// \param[in] lhs first parent
/// \param[in] rhs second parent
/// \return        crossover children (we only generate a single offspring)
///
/// We randomly select two loci (common crossover points). The offspring is
/// created with genes from the `rhs` parent before the first crossover
/// point and after the second crossover point; genes between crossover
/// points are taken from the `lhs` parent.
///
/// \note Parents must have the same size.
///
/// \relates ga::individual
///
individual crossover(const problem &,
                     const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  const auto ps(lhs.parameters());
  const auto cut1(random::sup(ps - 1));
  const auto cut2(random::between(cut1 + 1, ps));

  auto ret(rhs);
  for (auto i(cut1); i < cut2; ++i)
    ret.genome_[i] = lhs[i];  // not using `operator[](std::size_t)` to avoid
                              // multiple signature resets.

  ret.set_if_older_age(lhs.age());
  ret.signature_ = ret.hash();

  Ensures(ret.is_valid());
  return ret;
}

///
/// Hashes the current individual.
///
/// \return the hash value of the individual
///
/// Converts this individual in a packed representation (raw sequence of bytes)
/// and performs the *MurmurHash3* algorithm on it.
///
hash_t individual::hash() const
{
  const auto len(genome_.size() * sizeof(genome_[0]));  // length in bytes
  return len ? ultra::hash::hash128(genome_.data(), len) : hash_t();
}

///
/// Completely equivalent to `parameters()`.
///
/// \param[in] ind individual we're interested in
/// \return        number of parameters stored in the individual
///
std::size_t active_slots(const individual &ind) noexcept
{
  return ind.parameters();
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
      ultraERROR << "Empty individual must have empty signature";
      return false;
    }

    return true;
  }
  else /* !empty() */ if (signature() != hash())
  {
    ultraERROR << "Wrong signature: " << signature() << " should be "
               << hash();
    return false;
  }

  return true;
}

///
/// \param[in] in input stream
/// \return       `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful the current individual isn't
/// modified.
///
bool individual::load_impl(std::istream &in, const symbol_set &)
{
  std::size_t sz;
  if (!(in >> sz))
    return false;

  genome_t v(sz);
  for (auto &g : v)
    if (!(in >> g))
      return false;

  genome_ = v;
  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if the object has been saved correctly
///
bool individual::save_impl(std::ostream &out) const
{
  out << parameters() << '\n';
  for (const auto &g : genome_)
    out << g << '\n';

  return out.good();
}

///
/// \param[out] s  output stream
/// \param[in] ind individual to print
/// \return        output stream including `ind`
///
/// \relates ga::individual
///
std::ostream &operator<<(std::ostream &s, const individual &ind)
{
  return in_line(s, ind);
}

}  // namespace ultra::ga
