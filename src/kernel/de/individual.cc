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

#include <algorithm>

#include "kernel/de/individual.h"
#include "kernel/hash_t.h"
#include "kernel/random.h"

#include "utility/log.h"
#include "utility/misc.h"

namespace ultra::de
{

///
/// Constructs a new, random DE individual.
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
      return std::get<D_DOUBLE>(p.sset.roulette_terminal(n++));
    });

  Ensures(is_valid());
}

///
/// \return a const iterator pointing to the first gene
///
individual::const_iterator individual::begin() const
{
  return genome_.begin();
}

///
/// \return a const iterator pointing to (one) past the last gene
///
individual::const_iterator individual::end() const
{
  return genome_.end();
}

///
/// \return an iterator pointing to the first gene
///
individual::iterator individual::begin()
{
  return genome_.begin();
}

///
/// \return an iterator pointing to (one) past the last gene
///
individual::iterator individual::end()
{
  return genome_.end();
}

///
/// Returns a reference to the gene at specified location.
///
/// \param[in] i position of the gene to return
/// \return      the requested gene
///
/// \warning
/// Accessing a nonexistent element through this operator is undefined
/// behavior.
///
individual::value_type individual::operator[](std::size_t i) const
{
  Expects(i < parameters());
  return genome_[i];
}

individual::value_type &individual::operator[](std::size_t i)
{
  Expects(i < parameters());
  signature_.clear();
  return genome_[i];
}

///
/// This is sweet "syntactic sugar" to manage individuals as real value
/// vectors.
///
/// \return a vector of real values
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
  Expects(v.size() == parameters());

  genome_ = v;

  Ensures(is_valid());
  return *this;
}

///
/// Differential evolution crossover.
///
/// \param[in] p crossover probability (`parameters.evolution.p_cross`)
/// \param[in] f scaling factor interval (`parameters.de.weight`)
/// \param[in] a base vector
/// \param[in] b first vector used for calculating the difference vector
/// \param[in] c seconfo vector used for calculating the difference vector
/// \return      the offspring / trial vector
///
/// First the search direction is defined by calculating a *difference vector*
/// between the pair of vectors `b` and `c` (usually choosen at random from the
/// population). This difference vector is scaled by using the *scale factor*
/// `F`.
/// This way no separate probability distribution has to be used which makes
/// the scheme completely self-organizing.
///
/// The scaled difference vector is then added to a third vector `a`, called
/// the *base* vector. As a result a new vector is obtained, known as the
/// *mutant* or *donor* vector:
///
///     m = a + F * (b - c)
///
/// The mutation strategy used is named `DE/rand/1`.
/// Now the *offspring*, also called *trial*, vector is generated as follows:
///
///     offspring = crossover(this, m)
///
/// The mutant vector is recombined, based on a user defined parameter, called
/// *crossover probability*, with `this` (the *target* / *parent* vector).
/// The offspring gets **at least** one component of the mutant vector (the
/// last).
/// This scheme is called *binomial*.
///
/// `b` and `c` are used for mutation, `this` and `m` for crossover.
///
individual individual::crossover(double p, const interval<double> &f,
                                 const individual &a,
                                 const individual &b,
                                 const individual &c) const
{
  Expects(0.0 <= p && p <= 1.0);

  const auto ps(parameters());
  Expects(ps == a.parameters());
  Expects(ps == b.parameters());
  Expects(ps == c.parameters());

  // The weighting factor is randomly selected from an interval for each
  // difference vector (a technique called dither). Dither improves convergence
  // behaviour significantly, especially for noisy objective functions.
  const auto rf(random::element(f));

  auto ret(a);

  const auto last(ps - 1);
  for (std::size_t i(0); i < last; ++i)
    if (random::boolean(p))  // binomial scheme
      ret.genome_[i] += rf * (b[i] - c[i]);
    else
      ret.genome_[i] = operator[](i);

  // Last element is replaced for certain.
  ret.genome_[last] += rf * (b[last] - c[last]);

  ret.set_if_older_age(std::max({age(), a.age()}));

  ret.signature_.clear();
  Ensures(ret.is_valid());
  return ret;
}

///
/// \return `true` if the individual is empty, `false` otherwise
///
bool individual::empty() const noexcept
{
  return !parameters();
}

///
/// \return the number of parameters stored in the individual
///
std::size_t individual::parameters() const noexcept
{
  return genome_.size();
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
/// \return the signature of this individual
///
/// Identical individuals, at genotypic level, have same signature.
///
hash_t individual::signature() const
{
  if (signature_.empty())
    signature_ = hash();

  return signature_;
}

///
/// Hashes the current individual.
///
/// \return the signature of this individual
///
/// The signature is obtained performing *MurmurHash3* on the individual.
///
hash_t individual::hash() const
{
  const auto len(genome_.size() * sizeof(genome_[0]));
  return ultra::hash::hash128(genome_.data(), len);
}

///
/// Inserts into the output stream the graph representation of the individual.
///
/// \param[out] s  output stream
/// \param[in]  de data to be printed
///
/// \note
/// The format used to describe the graph is the dot language
/// (https://www.graphviz.org/).
///
/// \relates individual
///
std::ostream &graphviz(std::ostream &s, const individual &de)
{
  s << "graph {";

  for (const auto &g : de)
    s << "g [label=" << g << ", shape=circle];";

  s << '}';

  return s;
}

///
/// Prints the genes of the individual.
///
/// \param[out] s  output stream
/// \param[in]  de data to be printed
/// \return        a reference to the output stream
///
/// \relates individual
///
std::ostream &in_line(std::ostream &s, const individual &de)
{
  if (!de.empty())
  {
    s << *de.begin();

    for (auto it(std::next(de.begin())); it != de.end(); ++it)
      s << " " << *it;
  }

  return s;
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparsion
/// \return        a numeric measurement of the difference between `lhs` and
///                `rhs` (taxicab / L1 distance)
///
double distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0.0,
                            std::plus{},
                            [](auto a, auto b) { return std::fabs(a - b); });
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
      ultraERROR << "Empty individual must have empty signature";
      return false;
    }

    return true;
  }

  if (!signature_.empty() && signature_ != hash())
  {
    ultraERROR << "Wrong signature: " << signature_ << " should be " << hash();
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
    if (!load_float_from_stream(in, &g))
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
  for (const auto &v : genome_)
  {
    save_float_to_stream(out, v);
    out << '\n';
  }

  return out.good();
}

///
/// \param[out] s   output stream
/// \param[in]  ind individual to print
/// \return         output stream including `ind`
///
/// \relates individual
///
std::ostream &operator<<(std::ostream &s, const individual &ind)
{
  return in_line(s, ind);
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if the two individuals are equal
///
/// \note
/// Age is not checked.
///
bool operator==(const individual &lhs, const individual &rhs)
{
  return std::ranges::equal(lhs, rhs);
}

}  // namespace ultra::de
