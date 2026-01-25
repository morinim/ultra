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

///
/// Returns the currently selected dataset (const overload).
///
/// \return a const reference to the active dataset
///
template<DataSet D>
const D &multi_dataset<D>::selected() const noexcept
{
  return datasets_[as_integer(selected_)];
}

///
/// Returns the currently selected dataset.
///
/// \return a reference to the active dataset
///
template<DataSet D>
D &multi_dataset<D>::selected() noexcept
{
  return datasets_[as_integer(selected_)];
}

///
/// Accesses a dataset by role (const overload).
///
/// \param[in] d the dataset role to access
/// \return      a const reference to the requested dataset
///
/// \pre `d` must be a valid `dataset_t` value
///
template<DataSet D>
const D &multi_dataset<D>::operator[](dataset_t d) const noexcept
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  return datasets_[as_integer(d)];
}

///
/// Accesses a dataset by role.
///
/// \param[in] d the dataset role to access
/// \return      a reference to the requested dataset
///
/// \pre `d` must be a valid `dataset_t` value
///
template<DataSet D>
D &multi_dataset<D>::operator[](dataset_t d) noexcept
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  return datasets_[as_integer(d)];
}

///
/// Selects the active dataset.
///
/// \param[in] d the dataset to select
///
/// \pre `d` must be a valid `dataset_t` value
///
/// Subsequent calls to `selected()` will refer to the dataset identified by
/// `d`.
///
template<DataSet D>
void multi_dataset<D>::select(dataset_t d) noexcept
{
  Expects(dataset_t::training <= d && d <= dataset_t::test);
  selected_ = d;
}

#endif  // include guard
