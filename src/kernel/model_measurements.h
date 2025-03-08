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

#if !defined(ULTRA_MODEL_MEASUREMENTS_H)
#define      ULTRA_MODEL_MEASUREMENTS_H

#include <optional>

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

  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] friend bool operator==(
    const model_measurements &, const model_measurements &) noexcept = default;
  [[nodiscard]] friend bool operator!=(
    const model_measurements &, const model_measurements &) noexcept = default;

  // --- Serialization ---
  [[nodiscard]] bool load(std::istream &);
  [[nodiscard]] bool save(std::ostream &) const;

  std::optional<F> fitness {};
  std::optional<double> accuracy {};
};

template<Fitness F>
[[nodiscard]] auto operator<=>(const model_measurements<F> &,
                               const model_measurements<F> &) noexcept;

#include "kernel/model_measurements.tcc"

}  // namespace ultra

#endif  // include guard
