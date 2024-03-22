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

#if !defined(ULTRA_DE_INDIVIDUAL_H)
#define      ULTRA_DE_INDIVIDUAL_H

#include "kernel/individual.h"
#include "kernel/interval.h"
#include "kernel/problem.h"

namespace ultra::de
{

///
/// An individual optimized for differential evolution.
///
/// \see https://github.com/morinim/ultra/wiki/bibliography#4
///
class individual final : public ultra::individual
{
public:
  individual() = default;
  explicit individual(const ultra::problem &);

  // Iterators.
  using genome_t       = std::vector<double>;
  using const_iterator = genome_t::const_iterator;
  using iterator       = genome_t::iterator;
  using value_type     = genome_t::value_type;

  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] iterator begin();
  [[nodiscard]] iterator end();

  [[nodiscard]] value_type operator[](std::size_t) const;
  [[nodiscard]] value_type &operator[](std::size_t);

  [[nodiscard]] operator std::vector<value_type>() const noexcept;
  individual &operator=(const std::vector<value_type> &);

  [[nodiscard]] individual crossover(double, const interval_t<double> &,
                                     const individual &, const individual &,
                                     const individual &) const;

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t parameters() const noexcept;

  [[nodiscard]] hash_t signature() const;

  [[nodiscard]] bool is_valid() const;

private:
  // *** Private support methods ***
  [[nodiscard]] hash_t hash() const;

  // Serialization.
  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override;
  [[nodiscard]] bool save_impl(std::ostream &) const override;

  // *** Private data members ***

  // This is the genome: the entire collection of genes (the entirety of an
  // organism's hereditary information).
  genome_t genome_;
};  // class individual

[[nodiscard]] bool operator==(const individual &, const individual &);
[[nodiscard]] double distance(const individual &, const individual &);

// Visualization/output functions.
std::ostream &graphviz(std::ostream &, const individual &);
std::ostream &in_line(std::ostream &, const individual &);
std::ostream &operator<<(std::ostream &, const individual &);

}  // namespace ultra::de

#endif  // include guard
