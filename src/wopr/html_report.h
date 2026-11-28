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

#if !defined(ULTRA_WOPR_HTML_REPORT_H)
#define      ULTRA_WOPR_HTML_REPORT_H

#include "results.h"

#include <filesystem>
#include <string>

namespace ultra::wopr
{

/// Serializes a results snapshot into a self-contained HTML report powered by
/// Apache ECharts.
[[nodiscard]] std::string render_html_report(const results_snapshot &,
                                             bool = false,
                                             unsigned = 0);

/// Writes a report, embedding ECharts in the final output only.
[[nodiscard]] bool write_html_report(const std::filesystem::path &,
                                     const results_snapshot &,
                                     bool = false,
                                     unsigned = 0);

}  // namespace ultra::wopr

#endif  // include guard
