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

#if !defined(ULTRA_WOPR_DASHBOARD_H)
#define      ULTRA_WOPR_DASHBOARD_H

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace ultra::wopr
{

/// Describes one panel in a two-by-two dashboard.
struct dashboard_panel
{
  const char *id;
  const char *title;
  bool visible;
  std::function<void()> render;
};

/// Renders a two-by-two dashboard, optionally maximising one visible panel.
void render_dashboard(const std::array<dashboard_panel, 4> &,
                      std::optional<std::size_t> &);

/// Returns a retro pseudo-random string for status displays.
[[nodiscard]] std::string random_string();

}  // namespace ultra::wopr

#endif  // include guard
