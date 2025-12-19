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

#if !defined(ULTRA_GP_INDIVIDUAL_H)
#define      ULTRA_GP_INDIVIDUAL_H

#include <set>

#include "kernel/individual.h"
#include "kernel/problem.h"
#include "kernel/random.h"
#include "kernel/gp/gene.h"
#include "utility/matrix.h"
#include "utility/misc.h"

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

  // ---- Capacity ----
  [[nodiscard]] symbol::category_t categories() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] locus::index_t size() const noexcept;

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] const gene &operator[](const locus &) const;
  [[nodiscard]] locus start() const noexcept;

  [[nodiscard]] bool operator==(const individual &) const noexcept;

  [[nodiscard]] bool is_valid() const;

  // ---- Recombination operators ----
  enum crossover_t {one_point, two_points, tree, uniform, NUM_CROSSOVERS};

  friend individual crossover(const problem &,
                              const individual &, const individual &);
  unsigned mutation(const problem &);

  [[nodiscard]] crossover_t active_crossover_type() const noexcept;

  // ---- Iterators ----
  using const_exon_iterator = internal::basic_exon_iterator<true>;
  using exon_iterator = internal::basic_exon_iterator<false>;

  [[nodiscard]] basic_range<const_exon_iterator> cexons() const;

  [[nodiscard]] auto begin() const noexcept { return genome_.cbegin(); }
  [[nodiscard]] auto end() const noexcept { return genome_.cend(); }
  [[nodiscard]] auto begin() noexcept { return genome_.cbegin(); }

  template<bool> friend class internal::basic_exon_iterator;

private:
  // ---- Private member functions ----
  [[nodiscard]] basic_range<exon_iterator> exons();

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
[[nodiscard]] individual crossover(const problem &,
                                   const individual &, const individual &);
[[nodiscard]] unsigned distance(const individual &, const individual &);
[[nodiscard]] locus random_locus(const individual &);

std::ostream &operator<<(std::ostream &, const individual &);

}  // namespace ultra::gp

#endif  // include guard
