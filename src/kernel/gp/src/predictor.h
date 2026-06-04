/**
 *  \file
 *  \remark This file is part of ORACLE.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SRC_PREDICTOR_H)
#define      ULTRA_SRC_PREDICTOR_H

#include "kernel/gp/src/classification_result.h"
#include "kernel/gp/src/dataframe.h"

namespace ultra::src
{

///
/// Concept for regression predictors.
///
/// A regression predictor must be callable with an input example and return a
/// `value_t`.
///
template<class P> concept RegressionPredictor = requires(
  const P &p, const std::vector<value_t> &x)
{
  { p(x) } -> std::same_as<value_t>;
};

///
/// Concept for classification predictors.
///
/// A classification predictor must expose `tag()` returning a
/// `classification_result`.
///
template<class P> concept ClassificationPredictor = requires(
  const P &p, const std::vector<value_t> &x)
{
  { p.tag(x) } -> std::same_as<classification_result>;
};

}  // namespace ultra::src

#endif  // include guard
