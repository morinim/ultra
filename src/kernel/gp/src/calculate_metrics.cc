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

#include "kernel/gp/src/calculate_metrics.h"

namespace ultra::src
{

///
/// \param[in] o the model whose accuracy we are evaluating
/// \param[in] d a dataset
/// \return      the accuracy
///
double accuracy_metric::operator()(const core_reg_oracle *o,
                                   const dataframe &d) const
{
  Expects(!d.classes());
  Expects(d.size());

  const auto ok(
    std::ranges::count_if(
    d,
    [o](const auto &e)
    {
      const auto res((*o)(e.input));
      return has_value(res)
             && almost_equal(lexical_cast<D_DOUBLE>(res),
                             label_as<D_DOUBLE>(e));
    }));

  return static_cast<double>(ok) / static_cast<double>(d.size());
}

///
/// \param[in] o the model whose accuracy we are evaluating
/// \param[in] d a dataset
/// \return      the accuracy
///
double accuracy_metric::operator()(const core_class_oracle *o,
                                   const dataframe &d) const
{
  Expects(d.classes() >= 2);
  Expects(d.size());

  const auto ok(
    std::ranges::count_if(d,
                          [o](const auto &e)
                          {
                            return o->tag(e.input).label == label(e);
                          }));

  return static_cast<double>(ok) / static_cast<double>(d.size());
}

}  // namespace ultra::src
