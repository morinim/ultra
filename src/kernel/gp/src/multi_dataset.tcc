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
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_MULTI_DATASET_TCC)
#define      ULTRA_MULTI_DATASET_TCC

template<DataSet D>
const D &multi_dataset<D>::selected() const noexcept
{
  return datasets_[as_integer(selected_)];
}

template<DataSet D>
D &multi_dataset<D>::selected() noexcept
{
  return datasets_[as_integer(selected_)];
}

template<DataSet D>
const D &multi_dataset<D>::operator[](dataset_t d) const
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  return datasets_[as_integer(d)];
}

template<DataSet D>
D &multi_dataset<D>::operator[](dataset_t d)
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  return datasets_[as_integer(d)];
}

template<DataSet D>
void multi_dataset<D>::select(dataset_t d)
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  selected_ = d;
}

#endif  // include guard
