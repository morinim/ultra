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

// Includes the implementation to exercise its private snapshot and renderer.
#include "wopr/results.cc"

#include "implot/implot_internal.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace
{

using namespace ultra::wopr;

[[nodiscard]] results_state make_state(bool with_reference = true)
{
  summary_data reference;
  reference.runs = with_reference ? 20 : 0;
  reference.success_rate = 0.5;

  rs::collection_t collection;
  collection.emplace_back("Measured", rs::data{"", "", {}, reference});
  collection.back().second.current.runs = 10;
  collection.back().second.current.success_rate = 0.2;
  collection.emplace_back("Pending", rs::data{"", "", {}, {}});
  collection.emplace_back("Zero", rs::data{"", "", {}, reference});
  collection.back().second.current.runs = 5;
  return results_state(std::move(collection));
}

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

TEST_SUITE("WOPR RESULTS")
{

TEST_CASE("Missing success rates remain distinct from measured zero")
{
  auto state(make_state());
  const results_snapshot snapshot(state);

  REQUIRE(snapshot.success_rates.size() == 6);
  CHECK(snapshot.success_rates[0] == doctest::Approx(20.0));
  CHECK(std::isnan(snapshot.success_rates[1]));
  CHECK(snapshot.success_rates[2] == doctest::Approx(0.0));
  CHECK(snapshot.success_rates[3] == doctest::Approx(50.0));
  CHECK(std::isnan(snapshot.success_rates[4]));
  CHECK(snapshot.success_rates[5] == doctest::Approx(50.0));

  state.collection[1].second.current.runs = 1;
  const results_snapshot updated(state);
  CHECK(updated.success_rates[1] == doctest::Approx(0.0));
}

TEST_CASE("Success rate tooltips describe comparisons and missing results")
{
  auto state(make_state());
  const results_snapshot snapshot(state);

  const auto measured(success_rate_tooltip(snapshot, 0, true));
  CHECK(measured.starts_with("Measured\n"));
  CHECK(measured.contains("Current: 20.0% (10 completed runs)"));
  CHECK(measured.contains("Reference: 50.0% (20 completed runs)"));
  CHECK(measured.contains("Difference: -30.0 percentage points"));

  const auto pending(success_rate_tooltip(snapshot, 1, true));
  CHECK(pending.contains("Current: awaiting results"));
  CHECK(pending.contains("Reference: no reference"));
  CHECK_FALSE(pending.contains("Difference:"));

  const auto zero(success_rate_tooltip(snapshot, 2, true));
  CHECK(zero.contains("Current: 0.0% (5 completed runs)"));
  CHECK(zero.contains("Difference: -50.0 percentage points"));

  const auto current_only(success_rate_tooltip(snapshot, 0, false));
  CHECK_FALSE(current_only.contains("Reference:"));
  CHECK_FALSE(current_only.contains("Difference:"));

  state.collection[0].second.current.success_rate = 0.75;
  const results_snapshot improved(state);
  CHECK(success_rate_tooltip(improved, 0, true)
        .contains("Difference: +25.0 percentage points"));

  state.collection[0].second.current.runs = 0;
  const results_snapshot awaiting(state);
  const auto awaiting_text(success_rate_tooltip(awaiting, 0, true));
  CHECK(awaiting_text.contains("Current: awaiting results"));
  CHECK(awaiting_text.contains("Reference: 50.0% (20 completed runs)"));
  CHECK_FALSE(awaiting_text.contains("Difference:"));
}

TEST_CASE_FIXTURE(plot_context, "Success rate scale stays at zero to 100 percent")
{
  for (const bool with_reference : {true, false})
  {
    auto state(make_state(with_reference));
    for (const double rate : {0.0, 0.02, 1.0})
    {
      state.collection[0].second.current.success_rate = rate;
      const results_snapshot snapshot(state);
      ImGui::NewFrame();
      ImGui::SetNextWindowSize(ImVec2(900, 600));
      ImGui::Begin("Success rates");
      render_success_rate(snapshot);

      const auto *plot(ImPlot::GetPlot("##Success rate##Run"));
      REQUIRE(plot);
      const auto &axis(plot->Axes[ImAxis_Y1]);
      CHECK(axis.Range.Min == doctest::Approx(0.0));
      CHECK(axis.Range.Max == doctest::Approx(100.0));
      CHECK(axis.IsLocked());
      CHECK(std::string(axis.FormatSpec) == "%.0f%%");

      ImGui::End();
      ImGui::Render();
      for (const auto *list : ImGui::GetDrawData()->CmdLists)
        for (const auto &vertex : list->VtxBuffer)
        {
          CHECK(std::isfinite(vertex.pos.x));
          CHECK(std::isfinite(vertex.pos.y));
        }
    }
  }
}

}  // TEST_SUITE("WOPR RESULTS")
