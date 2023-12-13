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

#if !defined(ULTRA_MODEL_MEASUREMENTS_H)
#define      ULTRA_MODEL_MEASUREMENTS_H

#include "kernel/fitness.h"

namespace ultra
{
///
/// A collection of measurements.
///
template<Fitness F>
struct model_measurements
{
  model_measurements() = default;

  model_measurements(const F &, double);

  [[nodiscard]] friend auto operator<=>(const model_measurements &,
                                        const model_measurements &) = default;

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &);
  [[nodiscard]] bool save(std::ostream &) const;

  F      fitness  {};
  double accuracy {};
};

#include "kernel/model_measurements.tcc"

}  // namespace ultra

#endif  // include guard
