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

#if !defined(ULTRA_DISTRIBUTION_H)
#define      ULTRA_DISTRIBUTION_H

#include "utility/assert.h"
#include "utility/log.h"
#include "utility/misc.h"

#include <cmath>
#include <iomanip>
#include <map>

namespace ultra
{

///
/// Simplifies the calculation of statistics regarding a sequence (mean,
/// variance, standard deviation, min and max).
///
template<ArithmeticFloatingType T>
class distribution
{
public:
  distribution() = default;

  void clear();

  void add(T);

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] double entropy() const;
  [[nodiscard]] T max() const noexcept;
  [[nodiscard]] T mean() const noexcept;
  [[nodiscard]] T min() const noexcept;
  [[nodiscard]] const std::map<T, std::uintmax_t> &seen() const;
  [[nodiscard]] T standard_deviation() const;
  [[nodiscard]] T variance() const;

  void merge(const distribution<T> &);

  [[nodiscard]] bool is_valid() const;

  // ---- Serialization ----
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  // ---- Private member functions ----
  void update_variance(T);

  // ---- Private data members ----
  std::map<T, std::uintmax_t> seen_ {};

  T m2_ {};
  T m2_comp_ {};    // Kahan compensation
  T mean_ {};
  T mean_comp_ {};  // Kahan compensation
  T max_ {};
  T min_ {};

  std::size_t size_ {0};
};

#include "kernel/distribution.tcc"
}  // namespace ultra

#endif  // include guard
