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

#include "wopr/monitor.cc"

#include "implot/implot_internal.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace
{

using namespace ultra::wopr;

struct plot_context
{
  plot_context()
  {
    ImGui::CreateContext();
    ImPlot::CreateContext();
    auto &io(ImGui::GetIO());
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1000, 700);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  }

  ~plot_context()
  {
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
  }
};

}  // namespace

TEST_SUITE("WOPR MONITOR")
{

TEST_CASE_FIXTURE(plot_context, "render_layers_fit handles single layer without asserting")
{
  SUBCASE("many individuals")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 4318");
    const std::vector<layers_line> runs = {ll};

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Monitor test");
    CHECK_NOTHROW(render_layers_fit(runs));
    ImGui::End();
    ImGui::Render();
  }

  SUBCASE("single individual (parts == 1)")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 1");
    const std::vector<layers_line> runs = {ll};

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Monitor test");
    CHECK_NOTHROW(render_layers_fit(runs));
    ImGui::End();
    ImGui::Render();
  }

  SUBCASE("zero individuals")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 0");
    const std::vector<layers_line> runs = {ll};

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Monitor test");
    CHECK_NOTHROW(render_layers_fit(runs));
    ImGui::End();
    ImGui::Render();
  }
}

TEST_CASE_FIXTURE(plot_context, "render_layers_fit handles multiple layers")
{
  const layers_line ll(
    "0 0 1.0 0.5 1 2 0.5 0.1 0.3 0.7 50 10 2.0 0.8 1 3 0.8 0.2 0.6 0.9 60");
  const std::vector<layers_line> runs = {ll};

  ImGui::NewFrame();
  ImGui::SetNextWindowSize(ImVec2(900, 600));
  ImGui::Begin("Monitor test");
  CHECK_NOTHROW(render_layers_fit(runs));
  ImGui::End();
  ImGui::Render();
}

TEST_CASE_FIXTURE(plot_context, "render_layers_age handles single and multiple layers")
{
  SUBCASE("single layer")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 4318");
    const std::vector<layers_line> runs = {ll};

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Monitor test");
    CHECK_NOTHROW(render_layers_age(runs));
    ImGui::End();
    ImGui::Render();
  }

  SUBCASE("multiple layers")
  {
    const layers_line ll(
      "0 0 1.0 0.5 1 2 0.5 0.1 0.3 0.7 50 10 2.0 0.8 1 3 0.8 0.2 0.6 0.9 60");
    const std::vector<layers_line> runs = {ll};

    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Monitor test");
    CHECK_NOTHROW(render_layers_age(runs));
    ImGui::End();
    ImGui::Render();
  }
}

}  // TEST_SUITE
