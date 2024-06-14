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

#if !defined(ULTRA_GA_SEARCH_H)
#define      ULTRA_GA_SEARCH_H

#include "kernel/search.h"
#include "kernel/ga/problem.h"

namespace ultra::ga
{

///
/// Search driver for GAs.
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

#include "kernel/ga/search.tcc"

}  // namespace ultra::ga

#endif  // include guard
