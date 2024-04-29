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

#include "kernel/gp/src/oracle.h"

namespace ultra::src::serialize
{

///
/// Saves an oracle on persistent storage.
///
/// \param[in] out output stream
/// \param[in] ora oracle
/// \return        `true` on success
///
bool save(std::ostream &out, const basic_oracle &ora)
{
  out << ora.serialize_id() << '\n';;
  return ora.save(out);
}

bool save(std::ostream &out, const basic_oracle *ora)
{
  Expects(ora);
  return save(out, *ora);
}

bool save(std::ostream &out, const std::unique_ptr<basic_oracle> &ora)
{
  return save(out, ora.get());
}

namespace oracle::internal
{

std::map<std::string, build_func> factory_;

}  // namespace oracle::internal

}  // namespace ultra::src::serialize
