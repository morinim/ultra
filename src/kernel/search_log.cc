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

#include "kernel/search_log.h"

namespace ultra
{

std::filesystem::path search_log::build_path(
  const std::filesystem::path &f) const
{
  return f.is_absolute() ? f : base_dir / f;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool search_log::is_valid() const
{
  if (base_dir.has_filename())
  {
    ultraERROR << "Wrong base directory for search logs (contains the file `"
               << base_dir << "` instead of a directory)";
    return false;
  }

  if (!dynamic_file_path.empty() && !dynamic_file_path.has_filename())
  {
    ultraERROR << "`dynamic_file_path` must specify a file ("
               << dynamic_file_path << ")";
    return false;
  }

  if (!population_file_path.empty() && !population_file_path.has_filename())
  {
    ultraERROR << "`population_file_path` must specify a file ("
               << population_file_path << ")";
    return false;
  }

  if (!layers_file_path.empty() && !layers_file_path.has_filename())
  {
    ultraERROR << "`layers_file_path` must specify a file ("
               << layers_file_path << ")";
    return false;
  }

  if (!summary_file_path.empty() && !summary_file_path.has_filename())
  {
    ultraERROR << "`summary_file_path` must specify a file ("
               << summary_file_path << ")";
    return false;
  }

  return true;
}

bool search_log::open()
{
  if (!is_valid())
    return false;

  if (!dynamic_file.is_open())
  {
    const auto path(build_path(dynamic_file_path));
    dynamic_file.open(path, std::ios_base::app);
    if (!dynamic_file.is_open())
    {
      ultraWARNING << "Cannot open dynamic file " << path;
      return false;
    }
  }

  if (!population_file.is_open())
  {
    const auto path(build_path(population_file_path));
    population_file.open(path, std::ios_base::app);
    if (!population_file.is_open())
    {
      ultraWARNING << "Cannot open population file " << path;
      return false;
    }
  }

  if (!layers_file.is_open())
  {
    const auto path(build_path(layers_file_path));
    layers_file.open(path, std::ios_base::app);
    if (!layers_file.is_open())
    {
      ultraWARNING << "Cannot open layers file " << path;
      return false;
    }
  }

  return true;
}

}  // namespace ultra
