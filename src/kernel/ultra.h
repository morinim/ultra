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
 *
 *  Convenience header composed to be used by Ultra framework's clients.
 *  Ultra isn't Boost and a master header is a simplification for the API user.
 *  Anyway to reduce compile times in large projects, clients can include only
 *  individual headers.
 *
 *  \warning **NOT TO BE USED INTERNALLY**
 */

#if !defined(ULTRA_ULTRA_H)
#define      ULTRA_ULTRA_H

#include "kernel/de/individual.h"
#include "kernel/de/primitive.h"
#include "kernel/de/problem.h"
#include "kernel/de/search.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/src/dataframe.h"

#endif  // include guard
