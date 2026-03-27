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

#if !defined(ULTRA_DE_INDIVIDUAL_FORMAT_H)
#define      ULTRA_DE_INDIVIDUAL_FORMAT_H

#include "kernel/de/individual.h"
#include "kernel/individual_format.h"

namespace std
{

template<> struct formatter<ultra::de::individual, char>
  : ultra::internal::derived_individual_formatter<ultra::de::individual>
{
};

}  // namespace std

#endif  // include guard
