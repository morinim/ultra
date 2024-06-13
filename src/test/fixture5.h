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

#if !defined(ULTRA_FIXTURE5_H)
#define      ULTRA_FIXTURE5_H

#include "kernel/ga/problem.h"
#include "kernel/ga/primitive.h"

struct fixture5
{
  fixture5(unsigned n = 4)
  {
    prob.params.init();

    int v(10);
    for (unsigned i(0); i < n; ++i)
    {
      const auto r(ultra::interval(-v, +v));
      prob.insert(r);
      intervals.push_back(r);

      v *= 10;
    }
  }

  ultra::ga::problem prob {};
  std::vector<ultra::interval_t<int>> intervals {};
};

struct fixture5_no_init : fixture5
{
  fixture5_no_init() : fixture5(0) {}
};

#endif  // include guard
