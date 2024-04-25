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

#include "kernel/gp/src/search.h"

namespace ultra::src
{

search::search(problem &p, metric_flags m)
  : prob_(p), metrics_(m)
{
}

search_stats<search::individual_t, search::fitness_t> search::run(unsigned n)
{
  if (prob_.classification())
  {
    return {};
  }
  else
  {
    basic_search<alps_es, rmae_evaluator<gp::individual>> reg_search(
      prob_, rmae_evaluator<gp::individual>(prob_.data()), metrics_);

    return reg_search.run(n);
  }
}

}  // namespace ultra::src
