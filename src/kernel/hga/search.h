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

#if !defined(ULTRA_HGA_SEARCH_H)
#define      ULTRA_HGA_SEARCH_H

#include "kernel/search.h"
#include "kernel/hga/problem.h"

namespace ultra::hga
{

///
/// Search driver for HGAs.
///
/// This class implements ultra::search for GA optimization tasks.
///
template<Evaluator E>
class search : public ultra::basic_search<alps_es, E>
{
public:
  search(problem &, E);

protected:
  void tune_parameters() override;
};

#include "kernel/hga/search.tcc"

}  // namespace ultra::hga

#endif  // include guard
