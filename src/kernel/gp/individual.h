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

#include "kernel/gp/individual_iterator.tcc"

///
/// A single member of a genetic programming population.
///
/// Straight Line Program (SLP) is the encoding / data structure used to
/// represent the individual.
///
class individual final : public ultra::individual
{
public:
  individual() = default;
  explicit individual(const problem &);
  explicit individual(const std::vector<gene> &);

  [[nodiscard]] symbol::category_t categories() const;
  [[nodiscard]] bool empty() const;
  [[nodiscard]] locus::index_t size() const;

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] const gene &operator[](const locus &) const;
  [[nodiscard]] locus start() const;

  [[nodiscard]] bool operator==(const individual &) const;

  [[nodiscard]] bool is_valid() const;

  // ---- Recombination operators ----
  enum crossover_t {one_point, two_points, tree, uniform, NUM_CROSSOVERS};

  friend individual crossover(const individual &, const individual &);
  unsigned mutation(const problem &);

  // ---- Iterators ----
  using const_exon_iterator = internal::basic_exon_iterator<true>;
  using exon_iterator = internal::basic_exon_iterator<false>;

  using const_exon_range = internal::basic_exon_range<const_exon_iterator>;
  using exon_range = internal::basic_exon_range<exon_iterator>;

  [[nodiscard]] const_exon_range cexons() const;

  [[nodiscard]] auto begin() const { return genome_.begin(); }
  [[nodiscard]] auto end() const { return genome_.end(); }

  template<bool> friend class internal::basic_exon_iterator;

private:
  // ---- Private member functions ----
  [[nodiscard]] auto begin() { return genome_.begin(); }
  [[nodiscard]] exon_range exons();

  void pack(const locus &, std::vector<std::byte> *) const;
  [[nodiscard]] hash_t hash() const;

  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  // ---- Private data members ----
  // This is the genome: the entire collection of genes.
  matrix<gene> genome_ {};

  // Crossover operator used to create this individual. Initially this is set
  // to a random type.
  crossover_t active_crossover_type_ {random::sup(NUM_CROSSOVERS)};
};

[[nodiscard]] unsigned active_slots(const individual &);
[[nodiscard]] individual crossover(const individual &, const individual &);
[[nodiscard]] unsigned distance(const individual &, const individual &);
[[nodiscard]] locus random_locus(const individual &);

std::ostream &operator<<(std::ostream &, const individual &);

}  // namespace ultra::gp

#endif  // include guard
