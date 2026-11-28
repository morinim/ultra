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

#include "kernel/search_log.h"

#include <filesystem>

namespace ultra::wopr::monitor
{

struct options
{
  ultra::search_log slog {};
  unsigned window {0};
  std::filesystem::path html_output {"monitor.html"};
  unsigned interval {10};
};

[[nodiscard]] bool start(options);

}  // namespace ultra::wopr::monitor

#endif  // include guard
