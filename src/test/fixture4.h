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

#if !defined(ULTRA_FIXTURE4_H)
#define      ULTRA_FIXTURE4_H

#include "kernel/de/problem.h"
#include "kernel/de/primitive.h"

struct fixture4
{
  fixture4(unsigned n = 4)
  {
    prob.params.init();

    double v(10.0);
    for (unsigned i(0); i < n; ++i)
    {
      prob.insert(ultra::interval(-v, +v));
      v *= 10.0;
    }
  }

  ultra::de::problem prob {};
};

struct fixture4_no_init : fixture4
{
  fixture4_no_init() : fixture4(0) {}
};

#endif  // include guard
