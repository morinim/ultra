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

#if !defined(ULTRA_DISTRIBUTION_H)
#define      ULTRA_DISTRIBUTION_H

#include <cmath>
#include <iomanip>
#include <map>

#include "utility/assert.h"
#include "utility/log.h"
#include "utility/misc.h"

namespace ultra
{
///
/// Simplifies the calculation of statistics regarding a sequence (mean,
/// variance, standard deviation, entropy, min and max).
///
template<class T>
class distribution
{
public:
  distribution() = default;

  void clear();

  template<class U> void add(U);

  [[nodiscard]] std::uintmax_t size() const;
  [[nodiscard]] double entropy() const;
  [[nodiscard]] T max() const;
  [[nodiscard]] T mean() const;
  [[nodiscard]] T min() const;
  [[nodiscard]] const std::map<T, std::uintmax_t> &seen() const;
  [[nodiscard]] T standard_deviation() const;
  [[nodiscard]] T variance() const;

  bool is_valid() const;

public:   // Serialization
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  // Private methods.
  void update_variance(T);

  // Private data members.
  std::map<T, std::uintmax_t> seen_ {};

  T m2_ {};
  T max_ {};
  T mean_ {};
  T min_ {};

  std::uintmax_t size_ {0};
};

#include "kernel/distribution.tcc"
}  // namespace ultra

#endif  // include guard
