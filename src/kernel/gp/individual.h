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

#if !defined(ULTRA_GP_INDIVIDUAL_H)
#define      ULTRA_GP_INDIVIDUAL_H

#include <set>

#include "kernel/individual.h"
#include "kernel/problem.h"
#include "kernel/random.h"
#include "kernel/gp/gene.h"
#include "utility/matrix.h"

namespace ultra::gp
{

///
/// A single member of a genetic programming population.
///
/// Straight Line Program (SLP) is the encoding / data structure used to
/// represent the individual.
///
class individual final : public ultra::individual
{
public:
  //individual() = default;
  explicit individual(const problem &);
  explicit individual(const std::vector<gene> &);

  [[nodiscard]] std::size_t active_functions() const;
  [[nodiscard]] symbol::category_t categories() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] locus::index_t size() const;

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] const gene &operator[](const locus &) const;
  [[nodiscard]] locus start() const;

  [[nodiscard]] bool operator==(const individual &) const;

  [[nodiscard]] bool is_valid() const;

  // ---- Iterators ----
  template<bool> class basic_exon_iterator;
  using const_exon_iterator = basic_exon_iterator<true>;
  using exon_iterator = basic_exon_iterator<false>;

  template<class> class exons_range;
  [[nodiscard]] exons_range<const_exon_iterator> exons() const;
  [[nodiscard]] exons_range<exon_iterator> exons();

  template<bool> friend class basic_exon_iterator;

  // ---- Recombination operators ----
  enum crossover_t {one_point, two_points, tree, uniform, NUM_CROSSOVERS};

  unsigned mutation(double, const problem &);

private:
  // ---- Private data members ----
  void pack(const locus &, std::vector<std::byte> *) const;
  [[nodiscard]] hash_t hash() const;
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  // This is the genome: the entire collection of genes.
  matrix<gene> genome_;

  // Crossover operator used to create this individual. Initially this is set
  // to a random type.
  crossover_t active_crossover_type_ {random::sup(NUM_CROSSOVERS)};
};

[[nodiscard]] unsigned distance(const individual &, const individual &);

std::ostream &operator<<(std::ostream &, const individual &);

#include "kernel/gp/individual_iterator.tcc"

}  // namespace ultra::gp

#endif  // include guard
