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

#if !defined(ULTRA_MULTI_DATASET_H)
#define      ULTRA_MULTI_DATASET_H

#include "utility/assert.h"
#include "utility/misc.h"

#include <array>

namespace ultra::src
{

///
/// Identifies the role of a dataset within the learning pipeline.
///
/// Data/simulations are categorised into three disjoint sets:
/// - *training* used directly for learning;
/// - *validation* used to control overfitting and to measure performance;
/// - *test* used to estimate generalisation performance.
///
enum class dataset_t {training, validation, test};

///
/// A sized range modelling a dataset of examples.
///
template<class DAT> concept DataSet = std::ranges::sized_range<DAT>;

///
/// Container for multiple datasets with a selectable active one.
///
/// \tparam D a dataset type satisfying the `DataSet` concept
///
/// This class groups together a fixed set of datasets (training, validation,
/// and test) of the same type and provides convenient access to the currently
/// selected dataset.
///
template<DataSet D>
class multi_dataset
{
public:
  /// Constructs an empty multi-dataset.
  ///
  /// Each contained dataset is default-constructed. For typical container
  /// types, this results in empty datasets.
  multi_dataset() = default;

  // ---- Data access ----
  [[nodiscard]] const D &selected() const noexcept;
  [[nodiscard]] D &selected() noexcept;
  [[nodiscard]] const D &operator[](dataset_t) const noexcept;
  [[nodiscard]] D &operator[](dataset_t) noexcept;
  void select(dataset_t) noexcept;

private:
  // Number of datasets managed by this class.
  static constexpr std::size_t dataset_count = 3;
  static_assert(ultra::as_integer(dataset_t::test) + 1 == dataset_count,
                "dataset_t enumerators must be contiguous and zero-based");

  // Storage for the datasets, indexed by `dataset_t`.
  std::array<D, dataset_count> datasets_;

  // Currently selected dataset.
  dataset_t selected_ {dataset_t::training};
};  // class multi_dataset

#include "kernel/gp/src/multi_dataset.tcc"

}  // namespace ultra::src

#endif  // include guard
