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

namespace ultra
{

///
/// \param[in] force_defined all the undefined / auto-tuned parameters have to
///                          be in a "well defined" state for the function to
///                          pass the test
/// \return                  `true` if the object passes the internal
///                          consistency check
///
bool environment::is_valid(bool force_defined) const
{
  return true;
}

}  // namespace ultra
