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
  static constexpr unsigned integer_parameters = 4;
  static constexpr unsigned permutation_parameters = 1;
  static constexpr unsigned permutation_length = 32;
  static constexpr unsigned actual_length =
    integer_parameters + permutation_parameters * permutation_length;

  fixture6(const unsigned n = integer_parameters + permutation_parameters)
  {
    prob.params.init();

    if (n)
    {
      for (unsigned i(0); i < permutation_parameters; ++i)
        prob.insert<ultra::hga::permutation>(permutation_length);

      int v(10);
      for (unsigned i(0); i < integer_parameters; ++i)
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
