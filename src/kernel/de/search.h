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

#if !defined(ULTRA_DE_SEARCH_H)
#define      ULTRA_DE_SEARCH_H

#include "kernel/de/problem.h"
#include "kernel/search.h"

namespace ultra::de
{
///
/// Search driver for Differential Evolution.
///
/// This class implements ultra::search for DE optimization tasks.
///
template<Evaluator E>
class search : public ultra::search<de_es, E>
{
public:
  search(problem &, E);

protected:
  void tune_parameters() override;
};

#include "kernel/de/search.tcc"

}  // namespace ultra

#endif  // include guard
