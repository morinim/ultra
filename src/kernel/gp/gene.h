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

#include "kernel/gp/function.h"

namespace ultra
{
///
/// A gene is a unit of heredity in a living organism.
///
/// The class `gene` is the building block of a GP individual.
///
struct gene
{
public:
  // Types and constants.
  using arg_pack = std::vector<value_t>;

  gene() = default;
  gene(const function *, const arg_pack &);

  [[nodiscard]] bool is_valid() const;

  //locus locus_of_argument(std::size_t i) const;
  //small_vector<locus, K> arguments() const;

  // Public data members.
  const function *func;
  arg_pack        args;
};

[[nodiscard]] bool operator==(const gene &, const gene &);

}  // namespace ultra

#endif  // include guard
