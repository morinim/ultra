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

#include "kernel/parameters.h"
#include "utility/log.h"

namespace ultra
{

///
/// Initialises the undefined parameters with "common" values.
///
/// \return a reference to the "filled" `struct parameters`
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
parameters &parameters::init()
{
  slp.code_length = 100;

  population.individuals = 100;
  population.init_layers = 1;
  population.min_individuals = 2;

  evolution.brood_recombination = 1;
  evolution.elitism = 1.0;
  evolution.generations = 100;
  evolution.mate_zone = 20;
  evolution.max_stuck_gen = std::numeric_limits<unsigned>::max();
  evolution.p_cross = 0.9;
  evolution.p_mutation = 0.04;
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
bool parameters::is_valid(bool force_defined) const
{
  if (force_defined)
  {
    if (!alps.age_gap)
    {
      ultraERROR << "Undefined `age_gap` parameter";
      return false;
    }

    if (alps.p_main_layer < 0.0)
    {
      ultraERROR << "Undefined `p_main_layer` parameter";
      return false;
    }

    if (!evolution.brood_recombination)
    {
      ultraERROR << "Undefined `evolution.brood_recombination` data member";
      return false;
    }

    if (evolution.elitism < 0.0 || evolution.elitism > 1.0)
    {
      ultraERROR << "Undefined `evolution.elitism` data member";
      return false;
    }

    if (!evolution.generations)
    {
      ultraERROR << "Undefined `evolution.generations` data member";
      return false;
    }

    if (!evolution.mate_zone)
    {
      ultraERROR << "Undefined `evolution.mate_zone` data member";
      return false;
    }

    if (!evolution.max_stuck_gen)
    {
      ultraERROR << "Undefined `evolution.max_stuck_gen` data member";
      return false;
    }

    if (evolution.p_cross < 0.0)
    {
      ultraERROR << "Undefined `evolution.p_cross` data member";
      return false;
    }

    if (evolution.p_mutation < 0.0)
    {
      ultraERROR << "Undefined `evolution.p_mutation` data member";
      return false;
    }

    if (!evolution.tournament_size)
    {
      ultraERROR << "Undefined `evolution.tournament_size` data member";
      return false;
    }

    if (!population.individuals)
    {
      ultraERROR << "Undefined `population.individuals` data member";
      return false;
    }

    if (!population.init_layers)
    {
      ultraERROR << "Undefined `population.init_layers` data member";
      return false;
    }

    if (!population.min_individuals)
    {
      ultraERROR << "Undefined `population.min_individuals` data member";
      return false;
    }

    if (!slp.code_length)
    {
      ultraERROR << "Undefined `slp.code_length` data member";
      return false;
    }
  }  // if (force_defined)

  if (evolution.p_cross > 1.0)
  {
    ultraERROR << "`evolution.p_cross` out of range";
    return false;
  }

  if (evolution.p_mutation > 1.0)
  {
    ultraERROR << "`evolution.p_mutation` out of range";
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

  if (population.min_individuals == 1)
  {
    ultraERROR << "At least 2 individuals for layer";
    return false;
  }

  if (population.individuals && population.min_individuals
      && population.individuals < population.min_individuals)
  {
    ultraERROR << "`population.individuals` must be greater than or equal to "
               << "`population.min_individuals";
    return false;
  }

  if (population.individuals && evolution.tournament_size
      && evolution.tournament_size > population.individuals)
  {
    ultraERROR << "`evolution.tournament_size` (" << evolution.tournament_size
              << ") cannot be greater than population size ("
              << population.individuals << ")";
    return false;
  }

  if (de.weight.first > de.weight.second)
  {
    ultraERROR << "Wrong DE dither interval";
    return false;
  }

  if (alps.p_main_layer > 1.0)
  {
    ultraERROR << "`p_main_layer` out of range";
    return false;
  }

  if (stat.dir.has_filename())
  {
    ultraERROR << "`stat.dir` must contain a directory, not a file ("
               << stat.dir << ")";
    return false;
  }

  if (!stat.dynamic_file.empty() && !stat.dynamic_file.has_filename())
  {
    ultraERROR << "`stat.dynamic_file` must specify a file ("
               << stat.dynamic_file << ")";
    return false;
  }

  if (!stat.layers_file.empty() && !stat.layers_file.has_filename())
  {
    ultraERROR << "`stat.layers_file` must specify a file ("
               << stat.layers_file << ")";
    return false;
  }

  if (!stat.population_file.empty() && !stat.population_file.has_filename())
  {
    ultraERROR << "`stat.population_file` must specify a file ("
               << stat.population_file << ")";
    return false;
  }

  return true;
}

}  // namespace ultra
