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

#include "kernel/individual.h"
#include "kernel/decision_vector.h"
#include "kernel/problem.h"
#include "kernel/random.h"
#include "kernel/gp/gene.h"
#include "utility/matrix.h"
#include "utility/misc.h"

#include <iosfwd>
#include <set>

namespace ultra::gp
{

namespace internal
{
  class crossover_engine;
}

#include "kernel/gp/individual_exon_view.tcc"

///
/// Identifies one optimisable argument within a GP individual.
///
/// `loc` selects the gene and `arg_index` selects the argument within that
/// gene.
///
struct decision_coordinate
{
  locus loc;
  std::size_t arg_index;
};

/// GP-specific decision-vector type used for numerical optimisation.
using decision_vector = ultra::decision_vector<decision_coordinate>;


///
/// A single member of a genetic programming population.
///
/// Straight Line Program (SLP) is the encoding / data structure used to
/// represent the individual.
///
/// \note
/// This class implements a value type with no internal synchronisation. Any
/// operation that mutates the individual is not thread-safe and must not run
/// concurrently with any member function unless externally synchronised.
///
class individual final : public ultra::individual
{
public:
  // ---- Constructors ----
  individual() = default;
  explicit individual(const problem &);
  explicit individual(const std::vector<gene> &);

  // ---- Capacity ----
  [[nodiscard]] symbol::category_t categories() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] locus::index_t size() const noexcept;

  // ---- Element access ----
  [[nodiscard]] const gene &operator[](const locus &) const;
  [[nodiscard]] locus start() const noexcept;

  // ---- Recombination operators ----
  enum crossover_t {one_point, two_points, tree, uniform, NUM_CROSSOVERS};
  unsigned mutation(const problem &);

  void apply_decision_vector(const decision_vector &);

  [[nodiscard]] crossover_t active_crossover_type() const noexcept;

  // ---- Iterators ----
  using const_exon_iterator = internal::basic_exon_iterator<true>;
  using exon_iterator = internal::basic_exon_iterator<false>;

  [[nodiscard]] const_exon_view cexons() const;

  [[nodiscard]] auto begin() const noexcept { return genome_.cbegin(); }
  [[nodiscard]] auto end() const noexcept { return genome_.cend(); }

  // ---- Misc ----
  [[nodiscard]] bool operator==(const individual &) const noexcept;
  [[nodiscard]] bool is_valid() const;

private:
  template<bool> friend class internal::basic_exon_iterator;
  friend class internal::crossover_engine;

  // ---- Private member functions ----
  [[nodiscard]] exon_view exons();
  [[nodiscard]] hash_t hash() const override;
  void pack(const locus &, hash_sink &) const;

  // ---- Serialization ----
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  void print_impl(std::ostream &, out::print_format_t) const override;

  // ---- Private data members ----
  // This is the genome: the entire collection of genes.
  matrix<gene> genome_ {};

  // Crossover operator used to create this individual. Initially this is set
  // to a random type.
  crossover_t active_crossover_type_ {random::sup(NUM_CROSSOVERS)};
};


// ---- Non-member functions ----
[[nodiscard]] unsigned active_slots(const individual &);
[[nodiscard]] individual crossover(const problem &,
                                   const individual &, const individual &);
[[nodiscard]] unsigned distance(const individual &, const individual &);
[[nodiscard]] locus random_locus(const individual &);
[[nodiscard]] decision_vector extract_decision_vector(const individual &);

}  // namespace ultra::gp

#include "kernel/gp/individual_format.h"

#endif  // include guard
