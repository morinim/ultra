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

#include <csignal>

#if !defined(ULTRA_TERM_H)
#define      ULTRA_TERM_H

namespace ultra::term
{

void reset();
void set();
void signal_handler(int);
void term_raw_mode(bool);
[[nodiscard]] bool user_stop();

}  // namespace ultra::term

#endif  // include guard
