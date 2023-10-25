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
template<class T>
concept ArithmeticType =
  (std::is_floating_point_v<T>
   || std::is_floating_point_v<typename T::value_type>)
  && requires(T x, T y)
{
  {x + y};
  {x - y};
  {x * y};
  {x / y};
  {x < y};
  {x / 0.1};
};

///
/// Simplifies the calculation of statistics regarding a sequence (mean,
/// variance, standard deviation, entropy, min and max).
///
template<ArithmeticType T>
class distribution
{
public:
  distribution() = default;

  void clear();

  void add(T);

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] T max() const;
  [[nodiscard]] T mean() const;
  [[nodiscard]] T min() const;
  [[nodiscard]] T standard_deviation() const;
  [[nodiscard]] T variance() const;

  bool is_valid() const;

public:   // Serialization
  bool load(std::istream &);
  bool save(std::ostream &) const;

private:
  // Private methods.
  void update_variance(T);

  T m2_ {};
  T max_ {};
  T mean_ {};
  T min_ {};

  std::size_t size_ {0};
};

#include "kernel/distribution.tcc"
}  // namespace ultra

#endif  // include guard
