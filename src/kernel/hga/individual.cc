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
#include "kernel/print_internal.h"
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
individual::individual(const ultra::problem &p)
  : vector_individual(p.sset.categories())
{
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
/// Mutates the current individual.
///
/// \param[in] prb         the current problem
/// \param[in] temperature mutation intensity (`1.0` preserves the configured
///                        probability)
/// \return                number of mutations performed
///
/// \note
/// External parameters: `evolution.p_mutation`
///
unsigned individual::mutation(const problem &prb, double temperature)
{
  Expects(in_0_1(prb.params.evolution.p_mutation));
  Expects(temperature >= 0.0);

  const double pgm(
    1.0 - std::pow(1.0 - prb.params.evolution.p_mutation, temperature));

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
/// Calculates the Hamming distance between two individuals.
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
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
/// Heterogeneous crossover.
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

void individual::print_impl(std::ostream &s, out::print_format_t format) const
{
  internal::print_linear(s, *this, format, "hga::individual");
}

}  // namespace ultra::hga
