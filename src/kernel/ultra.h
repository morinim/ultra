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
#include "kernel/de/problem.h"
#include "kernel/de/search.h"
#include "kernel/ga/individual.h"
#include "kernel/ga/problem.h"
#include "kernel/ga/search.h"
#include "kernel/gp/individual.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/holdout_validation.h"
#include "kernel/gp/src/search.h"
#include "kernel/gp/src/variable.h"
#include "kernel/hga/individual.h"
#include "kernel/hga/problem.h"
#include "kernel/hga/search.h"

#endif  // include guard
