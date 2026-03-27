/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_GP_INDIVIDUAL_FORMAT_H)
#define      ULTRA_GP_INDIVIDUAL_FORMAT_H

#include "kernel/gp/individual.h"
#include "kernel/individual_format.h"

namespace std
{

template<> struct formatter<ultra::gp::individual, char>
  : formatter<ultra::individual, char>
{
  template<class FormatContext>
  auto format(const ultra::gp::individual &ind, FormatContext &ctx) const
  {
    using base = formatter<ultra::individual, char>;
    return base::format(ind, ctx);
  }
};

}  // namespace std

#endif  // include guard
