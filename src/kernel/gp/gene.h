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

#if !defined(ULTRA_GENE_H)
#define      ULTRA_GENE_H

#include "kernel/gp/locus.h"
#include "kernel/gp/function.h"

namespace ultra
{
///
/// A gene is the atomic unit of a genetic programming individual.
///
/// In ULTRA's GP engine, a gene represents a single instruction in a
/// *Straight Line Program (SLP)*. It consists of:
/// - a pointer to a function symbol (`func`);
/// - a fixed-size list of arguments (`args`) whose size equals the function
///   arity.
///
/// Arguments may be:
/// - terminals (constants, variables);
/// - addresses referring to genes at earlier loci, enforcing the acyclic,
///   feed-forward structure of SLPs.
///
/// ### Invariants
/// A `gene` is always in one of the following states:
/// - **empty gene**: `func == nullptr` and `args.empty()`;
/// - **active gene**: `func != nullptr` and `args.size() == func->arity()`.
///
/// These invariants are enforced by construction and validated by
/// `is_valid()`.
///
/// ### Ownership and lifetime
/// The `gene` class does **not** own the function it refers to. The pointed
/// `function` object is expected to be managed by a longer-lived
/// `symbol_set`.
///
/// ### Thread safety
/// `gene` is a passive value type and performs no internal synchronisation.
/// Concurrent access is safe only if no thread mutates the object.
///
/// \see gp::individual
///
struct gene
{
  // ---- Member types ----
  /// Type used to store the arguments of the function.
  using arg_pack = std::vector<value_t>;

  // ---- Constructors ----

  /// Constructs an empty (inactive) gene.
  ///
  /// The resulting gene has `func == nullptr` and no arguments. This state is
  /// valid and typically represents an unused slot in an individual's genome.
  gene() = default;

  gene(const function *, const arg_pack &);

  [[nodiscard]] locus locus_of_argument(std::size_t) const;
  [[nodiscard]] locus locus_of_argument(const arg_pack::value_type &) const;

  [[nodiscard]] symbol::category_t category() const;

  /// Equality comparison.
  ///
  /// Two genes are equal if they reference the same function symbol  and have
  /// identical arguments.
  [[nodiscard]] bool operator==(const gene &) const noexcept = default;

  [[nodiscard]] bool is_valid() const noexcept;

  // Public data members.
  const function *func {};  /// Pointer to the function symbol.
  arg_pack        args {};  /// Arguments of the function.
};

}  // namespace ultra

#endif  // include guard
