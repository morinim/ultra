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

[[nodiscard]] results_state make_state(
  bool with_reference = true, rs::exec_mode mode = rs::exec_mode::summary,
  bool with_threshold = false)
{
  summary_data reference;
  reference.runs = with_reference ? 20 : 0;
  reference.success_rate = 0.5;
  reference.elapsed_time = 90s;

  rs::settings settings;
  settings.runs = 20;
  if (with_threshold)
    settings.threshold.fitness = 0.0;

  rs::collection_t collection;
  collection.emplace_back("Measured", rs::data{"", "", settings, reference});
  collection.back().second.current.runs = 10;
  collection.back().second.current.success_rate = 0.2;
  collection.back().second.current.elapsed_time = 12500ms;
  collection.emplace_back("Pending", rs::data{"", "", {}, {}});
  collection.emplace_back("Zero", rs::data{"", "", {}, reference});
  collection.back().second.current.runs = 5;
  return results_state(std::move(collection), mode);
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

TEST_CASE("Summary files require valid XML and matching checksums")
{
  const fs::path path("wopr_summary_check.xml");
  struct cleanup
  {
    const fs::path &path;
    ~cleanup()
    {
      std::error_code error;
      fs::remove(path, error);
    }
  } guard{path};

  std::string xml(ultra::crc32::embed_xml_signature(
    "<ultra><summary><runs>2</runs><best><code>x &lt; y</code></best>"
    "</summary><checksum>00000000</checksum></ultra>"));
  bool valid(false);
  bool missing(false);

  SUBCASE("Valid summary") { valid = true; }
  SUBCASE("Tampered summary") { xml.replace(xml.find("<runs>2"), 7, "<runs>3"); }
  SUBCASE("Missing checksum") { xml = "<ultra><summary/></ultra>"; }
  SUBCASE("Malformed XML with a valid checksum")
  {
    xml = ultra::crc32::embed_xml_signature(
      "<ultra><summary></ultra><checksum>00000000</checksum>");
  }
  SUBCASE("Empty file") { xml.clear(); }
  SUBCASE("Missing file") { missing = true; }

  if (missing)
    fs::remove(path);
  else
  {
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out);
    out << xml;
    REQUIRE(out);
  }

  tinyxml2::XMLDocument doc;
  CHECK(summary_data::check(path, doc) == valid);
  if (valid)
  {
    const summary_data summary(doc);
    CHECK(summary.runs == 2);
    CHECK(summary.best_prg == "x < y");
  }
}

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

TEST_CASE("Runs without success thresholds use time per completed run")
{
  const auto check([](rs::exec_mode mode, bool with_threshold)
  {
    auto state(make_state(true, mode, with_threshold));
    const results_snapshot snapshot(state);

    CHECK(snapshot.show_success_rate
          == (mode == rs::exec_mode::summary || with_threshold));
    REQUIRE(snapshot.time_per_run.size() == 6);
    CHECK(snapshot.time_per_run[0] == doctest::Approx(1.25));
    CHECK(std::isnan(snapshot.time_per_run[1]));
    CHECK(snapshot.time_per_run[2] == doctest::Approx(0.0));
    CHECK(snapshot.time_per_run[3] == doctest::Approx(4.5));
    CHECK(std::isnan(snapshot.time_per_run[4]));

    if (mode == rs::exec_mode::run && with_threshold)
    {
      CHECK(snapshot.success_thresholds[0]);
      CHECK_FALSE(snapshot.success_thresholds[2]);
      CHECK(success_rate_tooltip(snapshot, 2, true)
            .contains("No success threshold defined"));
    }
  });

  check(rs::exec_mode::run, false);
  check(rs::exec_mode::run, true);
  check(rs::exec_mode::summary, false);

  rs::settings settings;
  settings.threshold.accuracy = 0.9;
  rs::collection_t collection;
  collection.emplace_back("Accuracy", rs::data{"", "", settings, {}});
  results_state state(std::move(collection), rs::exec_mode::run);
  CHECK(results_snapshot(state).show_success_rate);
}

TEST_CASE("Time per run tooltips describe timings and pending results")
{
  auto state(make_state());
  const results_snapshot snapshot(state);

  const auto measured(time_per_run_tooltip(snapshot, 0, true));
  CHECK(measured.starts_with("Measured\n"));
  CHECK(measured.contains("Current: 1.250 s/run (10 completed runs)"));
  CHECK(measured.contains("Reference: 4.500 s/run (20 completed runs)"));

  const auto pending(time_per_run_tooltip(snapshot, 1, true));
  CHECK(pending.contains("Current: awaiting results"));
  CHECK(pending.contains("Reference: no reference"));
  CHECK(time_per_run_tooltip(snapshot, 2, false)
        .contains("Current: 0.000 s/run (5 completed runs)"));
  CHECK_FALSE(time_per_run_tooltip(snapshot, 0, false).contains("Reference:"));
}

TEST_CASE("Remaining time estimates use each dataset's planned runs")
{
  rs::settings settings;
  settings.runs = 20;
  summary_data reference;
  reference.runs = 100;
  reference.elapsed_time = 100h;
  rs::collection_t collection;
  collection.emplace_back("Fast", rs::data{"", "", settings, reference});
  collection.emplace_back("Slow", rs::data{"", "", settings, {}});
  collection.emplace_back("Pending", rs::data{"", "", settings, {}});
  collection[0].second.current.runs = 10;
  collection[0].second.current.elapsed_time = 12500ms;
  collection[1].second.current.runs = 5;
  collection[1].second.current.elapsed_time = 2h;
  results_state state(std::move(collection), rs::exec_mode::run);
  const results_snapshot snapshot(state);

  REQUIRE(estimated_time_remaining(snapshot, 0));
  CHECK(*estimated_time_remaining(snapshot, 0) == doctest::Approx(12.5));
  CHECK(remaining_time_label(snapshot, 0) == "~13s");
  CHECK(remaining_time_label(snapshot, 1) == "~6h 0m 0s");
  CHECK_FALSE(estimated_time_remaining(snapshot, 2));
  CHECK(remaining_time_label(snapshot, 2) == "Pending");
  CHECK(time_per_run_tooltip(snapshot, 0, true)
        .contains("Runs left: 10\nEstimated time remaining: ~13s"));

  state.collection[0].second.current.runs = 15;
  state.collection[0].second.current.elapsed_time = 3min;
  const results_snapshot updated(state);
  CHECK(remaining_time_label(updated, 0) == "~1m 0s");

  for (const unsigned runs : {20u, 21u})
  {
    state.collection[0].second.current.runs = runs;
    const results_snapshot completed(state);
    REQUIRE(estimated_time_remaining(completed, 0));
    CHECK(*estimated_time_remaining(completed, 0) == doctest::Approx(0.0));
    CHECK(remaining_time_label(completed, 0) == "Complete");
    CHECK(time_per_run_tooltip(completed, 0, false).contains("Runs left: 0"));
  }
}

TEST_CASE_FIXTURE(plot_context, "Time per run plots use a non-negative time axis")
{
  for (const bool with_reference : {true, false})
  {
    auto state(make_state(with_reference, rs::exec_mode::run));
    SUBCASE("Completed results") {}
    SUBCASE("All results pending")
    {
      for (auto &[label, data] : state.collection)
        data.current.runs = 0;
    }
    const results_snapshot snapshot(state);
    for (unsigned frame(0); frame < 3; ++frame)
    {
      ImGui::NewFrame();
      ImGui::SetNextWindowSize(ImVec2(900, 600));
      ImGui::Begin("Run timings");
      render_time_per_run(snapshot);

      const auto *plot(ImPlot::GetPlot("##Time per run##Run"));
      REQUIRE(plot);
      CHECK(plot->Axes[ImAxis_X1].Range.Min >= 0.0);
      CHECK(plot->Axes[ImAxis_X1].Range.Max > 0.0);
      CHECK(plot->Axes[ImAxis_Y1].IsLocked());
      CHECK(plot->Axes[ImAxis_Y1].IsInverted());
      const auto &remaining_axis(plot->Axes[ImAxis_Y2]);
      CHECK(remaining_axis.Enabled);
      CHECK(remaining_axis.IsLocked());
      CHECK(remaining_axis.IsInverted());
      CHECK(remaining_axis.Range.Min
            == doctest::Approx(plot->Axes[ImAxis_Y1].Range.Min));
      CHECK(remaining_axis.Range.Max
            == doctest::Approx(plot->Axes[ImAxis_Y1].Range.Max));
      REQUIRE(remaining_axis.Ticker.TickCount() == snapshot.labels.size());
      for (std::size_t i(0); i < snapshot.labels.size(); ++i)
        CHECK(std::string(remaining_axis.Ticker.GetText(i))
              == remaining_time_label(snapshot, i));

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

TEST_CASE_FIXTURE(plot_context, "Run capacity lines stay opaque yellow")
{
  rs::settings settings;
  settings.runs = 20;
  rs::collection_t collection;
  collection.emplace_back("Dataset", rs::data{"", "", settings, {}});
  collection.back().second.current.runs = 10;
  results_state state(std::move(collection));
  const results_snapshot snapshot(state);

  for (unsigned frame(0); frame < 3; ++frame)
  {
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Run counts");
    render_number_of_runs(snapshot);
    ImGui::End();
    ImGui::Render();

    std::vector<ImVec2> line_uvs;
    for (const auto *list : ImGui::GetDrawData()->CmdLists)
      for (const auto &vertex : list->VtxBuffer)
        if (vertex.col == IM_COL32(255, 255, 0, 255))
          line_uvs.push_back(vertex.uv);

    REQUIRE(line_uvs.size() == 4);
    const auto *atlas(ImGui::GetIO().Fonts);
    auto *texture(atlas->TexData);
    REQUIRE(texture->Format == ImTextureFormat_RGBA32);
    const int x(static_cast<int>((line_uvs[0].x + line_uvs[2].x)
                                  * 0.5f * texture->Width));
    const int y(static_cast<int>((line_uvs[0].y + line_uvs[2].y)
                                  * 0.5f * texture->Height));
    REQUIRE(x >= 0);
    REQUIRE(x < texture->Width);
    REQUIRE(y >= 0);
    REQUIRE(y < texture->Height);
    const auto *pixel(static_cast<const unsigned char *>(
      texture->GetPixelsAt(x, y)));
    CHECK(pixel[0] == 255);
    CHECK(pixel[1] == 255);
    CHECK(pixel[2] == 255);
    CHECK(pixel[3] == 255);
  }
}

TEST_CASE_FIXTURE(plot_context, "Uniform heatmaps render with either axis direction")
{
  for (const auto x_flags : {ImPlotAxisFlags_None, ImPlotAxisFlags_Invert})
    for (const auto y_flags : {ImPlotAxisFlags_None, ImPlotAxisFlags_Invert})
    {
      ImGui::NewFrame();
      ImGui::SetNextWindowSize(ImVec2(900, 600));
      ImGui::Begin("Uniform fitness");
      if (ImPlot::BeginPlot("Heatmap"))
      {
        ImPlot::SetupAxes(nullptr, nullptr, x_flags, y_flags);
        ImPlot::SetupAxesLimits(0, 1, 0, 1, ImPlotCond_Always);
        ImPlot::SetupFinish();

        const double values[] = {1, 1, 1, 1};
        auto *draw_list(ImPlot::GetPlotDrawList());
        const int start(draw_list->VtxBuffer.Size);
        ImPlot::PlotHeatmap("Fitness", values, 2, 2, 1, 1, nullptr);
        CHECK(draw_list->VtxBuffer.Size > start);
        ImPlot::EndPlot();
      }
      else
        FAIL("Heatmap plot is not visible");
      ImGui::End();
      ImGui::Render();
    }
}

TEST_CASE_FIXTURE(plot_context, "Mean and best fitness markers stay distinct")
{
  int first_mean_count(0);
  for (unsigned frame(0); frame < 3; ++frame)
  {
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Fitness markers");
    if (ImPlot::BeginPlot("Fitness"))
    {
      ImPlot::SetupAxesLimits(0, 2, 0, 4, ImPlotCond_Always);
      ImPlot::SetupFinish();

      auto *draw_list(ImPlot::GetPlotDrawList());
      const int start(draw_list->VtxBuffer.Size);
      render_fitness_series("Current", 1, 1, 0.25, 3);
      const auto colour(ImPlot::GetItem("Current")->Color);
      const float middle_y(ImPlot::PlotToPixels(1, 2).y);
      int mean_count(0), best_count(0);
      for (int i(start); i < draw_list->VtxBuffer.Size; ++i)
        if (const auto &vertex(draw_list->VtxBuffer[i]); vertex.col == colour)
        {
          if (vertex.pos.y > middle_y)
            ++mean_count;
          else
            ++best_count;
        }
      CHECK(best_count > 0);
      CHECK(mean_count > best_count);
      if (!frame)
        first_mean_count = mean_count;
      CHECK(mean_count == first_mean_count);
      ImPlot::EndPlot();
    }
    else
      FAIL("Fitness plot is not visible");
    ImGui::End();
    ImGui::Render();
  }
}

TEST_CASE_FIXTURE(plot_context, "Plot fills apply opacity once")
{
  unsigned item(0);
  SUBCASE("Shaded region") { item = 0; }
  SUBCASE("Shaded line")   { item = 1; }
  SUBCASE("Shaded stairs") { item = 2; }
  SUBCASE("Bars")          { item = 3; }
  SUBCASE("Horizontal bars") { item = 4; }
  SUBCASE("Pre-step shaded stairs") { item = 5; }

  for (const bool per_point : {false, true})
  {
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(900, 600));
    ImGui::Begin("Fill opacity");
    if (ImPlot::BeginPlot("Fill"))
    {
      ImPlot::SetupAxesLimits(-1, 3, -1, 3);
      ImPlot::SetupFinish();
      const double xs[] = {0, 1, 2}, ys[] = {1, 2, 1};
      ImU32 colours[] = {IM_COL32_WHITE, IM_COL32_WHITE, IM_COL32_WHITE};
      ImPlotSpec spec(ImPlotProp_FillColor, ImVec4(1, 1, 1, 1),
                      ImPlotProp_FillAlpha, 0.25f,
                      ImPlotProp_LineWeight, 0.0f);
      if (per_point)
        spec.FillColors = colours;
      auto *draw_list(ImPlot::GetPlotDrawList());
      const int start(draw_list->VtxBuffer.Size);
      switch (item)
      {
      case 0:
        ImPlot::PlotShaded("Fill", xs, ys, 3, 0, spec);
        break;
      case 1:
        spec.Flags = ImPlotLineFlags_Shaded;
        ImPlot::PlotLine("Fill", xs, ys, 3, spec);
        break;
      case 2:
        spec.Flags = ImPlotStairsFlags_Shaded;
        ImPlot::PlotStairs("Fill", xs, ys, 3, spec);
        break;
      case 3:
        ImPlot::PlotBars("Fill", xs, ys, 3, 0.5, spec);
        break;
      case 4:
        spec.Flags = ImPlotBarsFlags_Horizontal;
        ImPlot::PlotBars("Fill", xs, ys, 3, 0.5, spec);
        break;
      case 5:
        spec.Flags = ImPlotStairsFlags_Shaded | ImPlotStairsFlags_PreStep;
        ImPlot::PlotStairs("Fill", xs, ys, 3, spec);
        break;
      }
      REQUIRE(draw_list->VtxBuffer.Size > start);
      for (int i(start); i < draw_list->VtxBuffer.Size; ++i)
        CHECK(draw_list->VtxBuffer[i].col == IM_COL32(255, 255, 255, 64));
      ImPlot::EndPlot();
    }
    else
      FAIL("Fill plot is not visible");
    ImGui::End();
    ImGui::Render();
  }
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
