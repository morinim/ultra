/**
 *  \file
 *  \remark This file is part of VITA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_ALPS_H)
#define      ULTRA_ALPS_H

#include <cstddef>

namespace ultra::alps
{

///
/// Parameters for the Age-Layered Population Structure (ALPS) paradigm.
///
/// ALPS is a meta heuristic for overcoming premature convergence by
/// running multiple instances of a search algorithm in parallel, with each
/// instance in its own age layer and having its own population.
///
struct parameters
{
  [[nodiscard]] unsigned allowed_age(unsigned, unsigned) const;
  [[nodiscard]] unsigned max_age(std::size_t) const;

  template<class T> [[nodiscard]] bool aged(const T &,
                                            unsigned, unsigned) const;

  /// The maximum ages for age layers is monotonically increasing and
  /// different methods can be used for setting these values. Since there is
  /// generally little need to segregate individuals which are within a few
  /// "generations" of each other, these values are then multiplied by an
  /// `age_gap` parameter. In addition, this allows individuals in the first
  /// age-layer some time to be optimized before them, or their offspring, are
  /// pushed to the next age layer.
  /// For instance, with 6 age layers, a linear aging-scheme and an age gap of
  /// 20, the maximum ages for the layers are: 20, 40, 60, 80, 100, 120.
  ///
  /// Also, the `age_gap` parameter sets the frequency of how often the first
  /// layer is restarted.
  ///
  /// \note A value of 0 means undefined (auto-tune).
  unsigned age_gap {20};

  /// The probability that a parent will be extracted from the main layer.
  ///
  /// \note
  /// A negative value means auto-tune.
  double p_main_layer {0.75};
};

}  // namespace vita::alps

#endif  // include guard
