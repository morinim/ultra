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

#include "kernel/hga/problem.h"

namespace ultra::hga
{

///
/// \return genome size / number of parameters / elements in the container
///
std::size_t problem::parameters() const noexcept
{
  return sset.categories();
}

}  // namespace ultra::hga
