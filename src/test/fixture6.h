/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_FIXTURE6_H)
#define      ULTRA_FIXTURE6_H

#include "kernel/hga/problem.h"
#include "kernel/hga/primitive.h"

struct fixture6
{
  fixture6(unsigned n = 5)
  {
    prob.params.init();

    if (n)
    {
      prob.insert<ultra::hga::permutation>(32);

      int v(10);
      for (unsigned i(1); i < n; ++i)
      {
        prob.insert<ultra::hga::integer>(ultra::interval(-v, v));

        v *= 10;
      }
    }
  }

  ultra::hga::problem prob {};
};

struct fixture6_no_init : fixture6
{
  fixture6_no_init() : fixture6(0) {}
};

#endif  // include guard
