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

#include "dashboard.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace ultra::wopr
{

void render_dashboard(const std::array<dashboard_panel, 4> &panels)
{
  const auto maximised(std::ranges::find_if(
    panels, [](const auto &panel)
    {
      return panel.visible && panel.maximised.get();
    }));

  std::array<bool, panels.size()> show {};
  std::ranges::transform(
    panels, show.begin(),
    [&maximised, &panels](const auto &panel)
    {
      return panel.visible
             && (maximised == panels.end() || &panel == &*maximised);
    });

  const auto available(ImGui::GetContentRegionAvail());
  const float available_width(available.x - 4.0f);
  const float available_height(available.y - 4.0f);

  for (std::size_t row(0); row < panels.size(); row += 2)
  {
    const bool row_visible(show[row] || show[row + 1]);
    if (!row_visible)
      continue;

    const std::size_t other_row(row ? 0 : 2);
    const bool other_row_visible(show[other_row] || show[other_row + 1]);

    const float width(show[row] && show[row + 1]
                      ? available_width / 2.0f : available_width);
    const float height(other_row_visible
                       ? available_height / 2.0f : available_height);

    bool first(true);
    for (std::size_t column(0); column < 2; ++column)
    {
      const std::size_t i(row + column);
      if (!show[i])
        continue;

      if (!first)
        ImGui::SameLine();
      first = false;

      const auto &panel(panels[i]);
      const auto panel_width(panel.maximised.get() ? available_width : width);
      const auto panel_height(panel.maximised.get() ? available_height
                                                    : height);

      ImGui::BeginChild(panel.id, ImVec2(panel_width, panel_height),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(panel.title);
      ImGui::SameLine();
      if (ImGui::Button(panel.maximised.get() ? "Minimise" : "Maximise"))
        panel.maximised.get() = !panel.maximised.get();

      panel.render();
      ImGui::EndChild();
    }
  }
}

}  // namespace ultra::wopr
