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

#if !defined(ULTRA_WOPR_MONITOR_H)
#define      ULTRA_WOPR_MONITOR_H

#include "imgui_app.h"

#include "kernel/search_log.h"

namespace argh { class parser; }

namespace ultra::wopr::monitor
{

extern ultra::search_log slog;
extern int window;

void start(const imgui_app::program::settings &);
[[nodiscard]] bool setup_cmd(argh::parser &);

}  // namespace ultra::wopr::monitor

#endif  // include guard
