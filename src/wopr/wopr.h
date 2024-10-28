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

#if !defined(ULTRA_WOPR_H)
#define      ULTRA_WOPR_H

#include <filesystem>

#include "utility/ts_queue.h"

void read_file(const std::filesystem::path &);

ultra::ts_queue<std::string> buffer_;

#endif  // include guard
