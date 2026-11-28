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

#if !defined(ULTRA_WOPR_HTML_MONITOR_H)
#define      ULTRA_WOPR_HTML_MONITOR_H

#include "monitor.h"
#include "monitor_data.h"

#include <filesystem>
#include <string>

namespace ultra::wopr
{

/// Writes a monitor page, embedding ECharts in the final output only.
[[nodiscard]] bool write_monitor_html(const std::filesystem::path &,
                                      const monitor_state &,
                                      const monitor::options &,
                                      bool = false);

}  // namespace ultra::wopr

#endif  // include guard
