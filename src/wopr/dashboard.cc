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

#include <array>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace ultra::wopr
{

void render_dashboard(const std::array<dashboard_panel, 4> &panels,
                      std::optional<std::size_t> &maximised)
{
  if (maximised && (*maximised >= panels.size() || !panels[*maximised].visible))
    maximised = std::nullopt;

  std::array<bool, panels.size()> show {};
  for (std::size_t i(0); i < panels.size(); ++i)
    show[i] = panels[i].visible && (!maximised || *maximised == i);

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

      ImGui::BeginChild(panel.id, ImVec2(width, height),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(panel.title);
      ImGui::SameLine();
      const bool is_maximised(maximised == i);
      if (ImGui::Button(is_maximised ? "Minimise" : "Maximise"))
        maximised = is_maximised ? std::nullopt : std::optional(i);

      panel.render();
      ImGui::EndChild();
    }
  }
}

std::string random_string()
{
  constexpr std::size_t length(10);

  static std::string_view charset =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  static std::minstd_rand g;

  static std::string fixed(length, char(0));
  static std::size_t fixed_count(0);

  if (fixed_count >= length)
  {
    fixed = std::string(length, char(0));
    fixed_count = 0;
  }

  std::string result;
  result.reserve(length);

  for (std::size_t i(0); i < length; ++i)
    result += fixed[i] ? fixed[i] : charset[g() % charset.size()];

  if (g() % 1000 == 0)
  {
    std::size_t next_fix(g() % length);
    while (fixed[next_fix])
      next_fix = (next_fix + 1) % length;

    fixed[next_fix] = result[next_fix];
    ++fixed_count;
  }

  return result;
}

}  // namespace ultra::wopr
