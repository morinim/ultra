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

#if !defined(ULTRA_CLASSIFICATION_RESULT_H)
#define      ULTRA_CLASSIFICATION_RESULT_H

#include <cstddef>

namespace ultra::src
{

/// The type used as class ID in classification tasks.
using class_t = std::size_t;

///
/// Contains a class ID / confidence level pair.
///
struct classification_result
{
  src::class_t label;   /// class ID
  double    sureness;   /// confidence level
};

}  // namespace ultra::src

#endif  // include guard
