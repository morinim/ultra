/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/environment.h"
#include "utility/log.h"

namespace ultra
{

///
/// Initialises the undefined parameters with "common" values.
///
/// \return a reference to the "filled" environment
///
/// Usually:
/// - the undefined parameters are tuned before the start of the search
///   (search::run calls search::tune_parameters) when there are enough data at
///   hand;
/// - the user doesn't have to fiddle with them (except after careful
///   consideration).
///
/// This function is mainly convenient for debugging purpose. The chosen values
/// are reasonable but most likely far from ideal.
///
/// \see search::tune_parameters
///
environment &environment::init()
{
  slp.code_length = 100;

  population.individuals = 100;
  population.layers = 1;

  evolution.mate_zone = 20;
  evolution.tournament_size = 5;

  return *this;
}

///
/// \param[in] force_defined all the undefined / auto-tuned parameters have to
///                          be in a "well defined" state for the function to
///                          pass the test
/// \return                  `true` if the object passes the internal
///                          consistency check
///
bool environment::is_valid(bool force_defined) const
{
  if (force_defined)
  {
    if (!slp.code_length)
    {
      ultraERROR << "Undefined `slp.code_length` data member";
      return false;
    }

    if (!population.individuals)
    {
      ultraERROR << "Undefined `population.individuals` data member";
      return false;
    }

    if (!population.layers)
    {
      ultraERROR << "Undefined population.layers data member";
      return false;
    }

    if (!evolution.mate_zone)
    {
      ultraERROR << "Undefined mate_zone data member";
      return false;
    }

    if (!evolution.tournament_size)
    {
      ultraERROR << "Undefined tournament_size data member";
      return false;
    }

    if (evolution.mate_zone && evolution.tournament_size
        && evolution.tournament_size > evolution.mate_zone)
    {
      ultraERROR << "`tournament_size` (" << evolution.tournament_size
                 << ") cannot be greater than `mate_zone` ("
                 << evolution.mate_zone << ")";
      return false;
    }

  }  // if (force_defined)

  if (population.individuals && evolution.tournament_size
      && evolution.tournament_size > population.individuals)
  {
    ultraERROR << "`tournament_size` (" << evolution.tournament_size
              << ") cannot be greater than population size ("
              << population.individuals << ")";
    return false;
  }

  return true;
}

}  // namespace ultra
