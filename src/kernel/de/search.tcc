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
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_DE_SEARCH_TCC)
#define      ULTRA_DE_SEARCH_TCC

///
/// Search class specialization DE.
///
template<Evaluator E>
search<E>::search(problem &prob, E eva)
  : ultra::basic_search<de_es, E>(prob, eva)
{
}

///
/// Tries to tune search parameters for the current function.
///
template<Evaluator E>
void search<E>::tune_parameters()
{
  ultra::basic_search<de_es, E>::tune_parameters();

  if (this->prob_.params.population.min_individuals < 10)
    this->prob_.params.population.min_individuals = 10;

  Ensures(this->prob_.params.is_valid(true));
}

#endif  // include guard
