/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/hga/individual.h"
#include "kernel/hga/primitive.h"
#include "kernel/hash_t.h"
#include "kernel/random.h"

#include "utility/log.h"
#include "utility/misc.h"

#include <algorithm>

namespace ultra::hga
{

///
/// Constructs a new, random HGA individual.
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
      assert(p.sset.terminals(n) == 1);
      return p.sset.front_terminal(n++)->instance();
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
/// behaviour.
///
const individual::value_type &individual::operator[](std::size_t i) const
{
  Expects(i < size());
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
  Expects(v.size() == parameters());

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
  {
    const auto *sym(prb.sset.front_terminal(c));

    if (is<integer>(sym))
    {
      if (random::boolean(pgm))
        if (const auto g(prb.sset.front_terminal(c)->instance());
            g != genome_[c])
        {
          ++n;
          genome_[c] = g;
        }
    }
    else if (is<permutation>(sym))
    {
      auto &vec(std::get<D_IVECTOR>(genome_[c]));
      const auto v_size(vec.size());

      for (std::size_t i(0); i < v_size/2; ++i)
        if (random::boolean(pgm))
          if (const auto rand(random::sup(v_size)); rand != i)
          {
            std::swap(vec[i], vec[rand]);
            ++n;
          }
    }
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
///                `rhs`
///
/// \note
/// Genes of the `D_IVECTOR` size are compared element by element. So the
/// distance between `{1, 2, {1, 2, 3}}` and `{0, 2, {0, 3, 1}}` is `4` (and
/// not `2`.
///
/// \relates hga::individual
///
unsigned distance(const individual &lhs, const individual &rhs)
{
  Expects(lhs.size() == rhs.size());
  Expects(std::ranges::equal(lhs, rhs,
                             [](auto l, auto r)
                             {
                               return l.index() == r.index();
                             }));

  return std::inner_product(
    lhs.begin(), lhs.end(), rhs.begin(), 0u,
    std::plus{},
    [](const auto &v1, const auto &v2)
    {
      if (v1.index() == d_ivector)
        return hamming_distance(std::get<D_IVECTOR>(v1),
                                std::get<D_IVECTOR>(v2));

      return static_cast<unsigned>(v1 != v2);
    });
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if the two individuals are equal
///
/// \note
/// Age isn't checked.
///
/// \relates hga::individual
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
/// \relates hga::individual
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
/// \relates individual
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
/// Partially mapped  crossover (PMX).
///
/// \param[in] lhs first permutation
/// \param[in] rhs second permutation
/// \return        new permutation
///
/// The Partially Mapped Crossover (PMX) is a recombination operator, initially
/// designed for TSP like problems, that utilizes the genetic material of two
/// parent solutions to propose a new offspring.
/// It is one of the most commonly used crossover operator for
/// permutation-encoded chromosomes.
/// The principle behind PMX is to preserve the arrangement of genes from a
/// parent while allowing variation in genes.
///
/// \relates individual
///
D_IVECTOR pmx(const D_IVECTOR &lhs, const D_IVECTOR &rhs)
{
  Expects(std::ranges::is_permutation(lhs, rhs));

  const auto ps(lhs.size());
  const auto cut1(random::sup(ps - 1));
  const auto cut2(random::between(cut1 + 1, ps));

  auto ret(lhs);

  const std::vector<std::pair<std::size_t, std::size_t>> segments =
    {{0, cut1}, {cut2, ps}};

  for (const auto &segment : segments)
    for (auto s(segment.first); s < segment.second; ++s)
    {
      auto candidate(rhs[s]);

      auto j(cut1);
      while (j < cut2)
        if (candidate != ret[j])
          ++j;
        else
        {
          candidate = rhs[j];
          j = cut1;
        }

      ret[s] = candidate;
    }

  return ret;
}

///
/// Heterogeneos crossover.
///
/// \param[in] prb the current problem
/// \param[in] lhs first parent
/// \param[in] rhs second parent
/// \return        crossover children (we only generate a single offspring)
///
/// Genes of class `integer` are crossed using homogenous crossover; genes of
/// class `permutation` are recombinex using PMX.
///
/// \note Parents must have the same size.
///
/// \relates individual
///
individual crossover(const problem &prb,
                     const individual &lhs, const individual &rhs)
{
  Expects(lhs.parameters() == rhs.parameters());

  individual ret(lhs);

  const auto ps(lhs.parameters());
  for (std::size_t i(0); i < ps; ++i)
  {
    const auto *sym(prb.sset.front_terminal(i));
    if (is<integer>(sym))
    {
      if (random::boolean())
        ret.genome_[i] = rhs[i];
    }
    else if (is<permutation>(sym))
    {
      assert(lhs[i].index() == d_ivector);
      assert(rhs[i].index() == d_ivector);
      ret.genome_[i] = pmx(std::get<D_IVECTOR>(lhs[i]),
                           std::get<D_IVECTOR>(rhs[i]));
    }
  }

  ret.set_if_older_age(rhs.age());
  ret.signature_ = ret.hash();

  Ensures(ret.is_valid());
  return ret;
}

///
/// Maps individuals to a byte stream.
///
/// \return a byte stream compacted version of the gene sequence
///
/// Useful for individual comparison / information retrieval.
///
std::vector<std::byte> individual::pack() const
{
  std::vector<std::byte> ret;

  const auto pack_span([&ret](const std::span<const std::byte> data)
  {
    ret.insert(ret.end(), data.begin(), data.end());
  });

  const auto pack_value([&](const auto &data)
  {
    pack_span(bytes_view(data));
  });

  for (const auto &v : genome_)
    switch (v.index())
    {
    case d_double:
      pack_value(std::get<D_DOUBLE>(v));
      break;
    case d_int:
      pack_value(std::get<D_INT>(v));
      break;
    case d_ivector:
      for (const auto &elem : std::get<D_IVECTOR>(v))
        pack_value(elem);
      break;
    case d_string:
      pack_value(std::get<D_STRING>(v));
      break;
    case d_void:
      break;
    }

  return ret;
}

///
/// Hashes the current individual.
///
/// \return the hash value of the individual
///
hash_t individual::hash() const
{
  const auto packed(pack());
  return packed.size() ? ultra::hash::hash128(packed.data(), packed.size())
                       : hash_t();
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
/// \param[in] ss symbol set (currently not used since terminals used in HGAs
///               don't require deconding)
/// \return       `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful the current individual isn't
/// modified.
///
bool individual::load_impl(std::istream &in, const symbol_set &ss)
{
  std::size_t sz;
  if (!(in >> sz))
    return false;

  genome_t v(sz);
  for (auto &g : v)
    if (!ultra::load(in, ss, g))
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
  {
    ultra::save(out, g);
    out << '\n';
  }

  return out.good();
}

///
/// \param[out] s  output stream
/// \param[in] ind individual to print
/// \return        output stream including `ind`
///
/// \relates individual
///
std::ostream &operator<<(std::ostream &s, const individual &ind)
{
  return in_line(s, ind);
}

}  // namespace ultra::hga
