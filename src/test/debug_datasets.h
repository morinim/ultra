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

#include <string>

#if !defined(ULTRA_DEBUG_DATASETS_H)
#define      ULTRA_DEBUG_DATASETS_H

namespace ultra::debug
{

constexpr std::size_t SR_COUNT = 10;

constexpr char sr[] = R"(
      95.2425,  2.81
    1554,       6
    2866.5485,  7.043
    4680,       8
   11110,      10
   18386.0340, 11.38
   22620,      12
   41370,      14
   54240,      15
  168420,      20
)";

}  // namespace ultra::debug

#endif  // include guard
