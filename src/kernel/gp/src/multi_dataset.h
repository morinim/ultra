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

#include <vector>

#include "utility/assert.h"
#include "utility/misc.h"

namespace ultra::src
{

/// Data/simulations are categorised in three sets:
/// - *training* used directly for learning;
/// - *validation* for controlling overfitting and measuring the performance
///   of an individual;
/// - *test* for a forecast of how well an individual will do in the real
///   world.
enum class dataset_t {training, validation, test};

///
/// A sized range that contains a series of examples.
///
template<class DAT> concept DataSet = std::ranges::range<DAT> && requires(DAT d)
{
  typename DAT::difference_type;
};

template<DataSet D>
class multi_dataset
{
public:
  explicit multi_dataset() = default;

  // ---- Data access ----
  [[nodiscard]] const D &selected() const noexcept;
  [[nodiscard]] D &selected() noexcept;
  [[nodiscard]] const D &operator[](dataset_t) const;
  [[nodiscard]] D &operator[](dataset_t);
  void select(dataset_t);

private:
  std::vector<D> datasets_ { {}, {}, {} };
  dataset_t selected_ {dataset_t::training};
};  // class multi_dataset

#include "kernel/gp/src/multi_dataset.tcc"

}  // namespace ultra::src

#endif  // include guard
