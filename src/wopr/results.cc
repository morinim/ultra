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

#include "results.h"

#include "dashboard.h"
#include "gui_helpers.h"

#include "kernel/search_log.h"

#include "implot/implot.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <format>
#include <fstream>
#include <mutex>
#include <optional>
#include <ranges>
#include <shared_mutex>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace ultra::wopr
{

using namespace std::chrono_literals;

namespace rs
{

}  // namespace rs

const std::string current_str {"Current"};
const std::string reference_str {"Reference"};

using labels_data = std::vector<const char *>;

struct results_state
{
  explicit results_state(rs::collection_t c,
                         rs::exec_mode m = rs::exec_mode::summary)
    : collection(std::move(c)), mode(m) {}

  rs::collection_t collection;
  const rs::exec_mode mode;
  std::shared_mutex current_mutex;
};

[[nodiscard]] std::vector<double> make_positions(std::size_t);
[[nodiscard]] std::vector<const char *> to_cstr_vector(
  const std::vector<std::string> &);

struct fit_data
{
  explicit fit_data(std::size_t count)
  {
    best.reserve(count);
    mean.reserve(count);
    std_dev.reserve(count);
    runs.reserve(count);
  }

  std::vector<double> best;
  std::vector<double> mean;
  std::vector<double> std_dev;
  std::vector<double> runs;
};

struct elite_data
{
  explicit elite_data(std::size_t count) { elite.reserve(count); }

  struct run
  {
    std::vector<unsigned> id {};
    std::vector<double> fit {};
    std::vector<double> acc {};
    std::vector<std::string> tick_text {};
    labels_data tick_labels {};
    std::vector<double> tick_positions {};
  };

  std::vector<run> elite;
};

struct results_snapshot
{
  explicit results_snapshot(results_state &);

  std::vector<std::string> labels;
  labels_data label_data;
  std::vector<double> positions;
  bool references_available {false};

  std::vector<unsigned> capacity_runs;
  std::vector<unsigned> runs;
  unsigned max_runs {0};
  unsigned max_runs_with_reference {0};
  bool show_success_rate {false};
  std::vector<bool> success_thresholds;
  std::vector<double> success_rates;
  std::vector<double> time_per_run;
  fit_data current_fitness;
  fit_data reference_fitness;
  elite_data current_elite;
  elite_data reference_elite;
};

bool summary_data::check(const fs::path &xml_fn, tinyxml2::XMLDocument &doc)
{
  std::ifstream in(xml_fn, std::ios::binary);
  if (!in)
    return false;

  std::ostringstream ss;
  if (!(ss << in.rdbuf()))
    return false;

  const std::string xml_content(ss.str());

  return ultra::crc32::verify_xml_signature(xml_content)
         && doc.Parse(xml_content.data(), xml_content.size())
            == tinyxml2::XML_SUCCESS
         && doc.FirstChild();
}

summary_data::summary_data(const fs::path &path)
{
  if (tinyxml2::XMLDocument doc; check(path, doc))
    *this = summary_data(doc);
  else
    throw std::invalid_argument("Cannot parse summary file \""
                                + path.generic_string() + "\"");
}

summary_data::summary_data(const tinyxml2::XMLDocument &doc)
{
  tinyxml2::XMLConstHandle handle(&doc);

  const auto h_summary(handle
                       .FirstChildElement("ultra")
                       .FirstChildElement("summary"));
  if (const auto *e = h_summary.FirstChildElement("runs").ToElement())
    runs = e->UnsignedText(0);
  if (const auto *e = h_summary.FirstChildElement("elapsed_time").ToElement())
    elapsed_time = std::chrono::milliseconds(e->UnsignedText(0));
  if (const auto *e = h_summary.FirstChildElement("success_rate").ToElement())
    success_rate = e->DoubleText(-1.0);

  {
    const auto h_distributions(h_summary
                               .FirstChildElement("distributions"));
    if (const auto *e = h_distributions.FirstChildElement("fitness")
                        .FirstChildElement("mean").ToElement())
    {
      std::istringstream ss(e->GetText());
      ss >> fit_mean;
    }
    if (const auto *e = h_distributions.FirstChildElement("fitness")
                        .FirstChildElement("standard_deviation").ToElement())
    {
      std::istringstream ss(e->GetText());
      ss >> fit_std_dev;
    }
  }

  {
    const auto h_best(h_summary.FirstChildElement("best"));
    if (const auto *e = h_best.FirstChildElement("fitness").ToElement())
    {
      std::istringstream ss(e->GetText());
      ss >> best_fit;
    }
    if (const auto *e = h_best.FirstChildElement("accuracy").ToElement())
      best_accuracy = e->DoubleText(0.0);
    if (const auto *e = h_best.FirstChildElement("run").ToElement())
      best_run = e->UnsignedText(0);
    if (const auto *e = h_best.FirstChildElement("code").ToElement())
      best_prg = e->GetText();
  }

  {
    const auto h_solutions(h_summary.FirstChildElement("solutions"));

    for (const auto *e(h_solutions.FirstChildElement().ToElement()); e;
         e = e->NextSiblingElement())
      if (unsigned run; e->QueryUnsignedText(&run) == tinyxml2::XML_SUCCESS)
        good_runs.insert(run);
  }

  {
    const auto h_elite(h_summary.FirstChildElement("elite"));

    auto auto_id(std::numeric_limits<decltype(runs)>::max() / 2);

    if (const auto *elite_el = h_elite.ToElement())
    {
      // unsigned percentile = elite_el->UnsignedAttribute("percentile", 0);

      for (const auto *run_el(elite_el->FirstChildElement("run"));
           run_el;
           run_el = run_el->NextSiblingElement("run"))
      {
        unsigned id;
        if (const auto id_rc(run_el->QueryUnsignedAttribute("id", &id));
            id_rc != tinyxml2::XML_SUCCESS)
          id = ++auto_id;  // fallback

        ultra::model_measurements<ultra::fitnd> mm;

        if (const auto *e = run_el->FirstChildElement("fitness"))
          if (const char *txt = e->GetText())
          {
            std::istringstream ss(txt);
            if (ultra::fitnd f; ss >> f)
              mm.fitness = std::move(f);
          }

        if (const auto *e = run_el->FirstChildElement("accuracy"))
          mm.accuracy = e->DoubleText(0.0);

        elite.emplace_back(id, std::move(mm));
      }
    }
  }
}

void add_fit_data(fit_data &out, const summary_data &summary)
{
  constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

  out.best.push_back(summary.best_fit.size() ? summary.best_fit[0] : qnan);
  out.mean.push_back(summary.fit_mean.size() ? summary.fit_mean[0] : qnan);
  out.std_dev.push_back(summary.fit_std_dev.size()
                        ? summary.fit_std_dev[0] : qnan);
  out.runs.push_back(summary.runs);
}

void add_elite_data(elite_data &out, const summary_data &summary)
{
  constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

  elite_data::run run;
  run.id.reserve(summary.elite.size());
  run.fit.reserve(summary.elite.size());
  run.acc.reserve(summary.elite.size());

  for (const auto &[id, measurements] : summary.elite)
  {
    run.id.push_back(id);
    run.fit.push_back(measurements.fitness
                      ? (*measurements.fitness)[0] : qnan);
    run.acc.push_back(measurements.accuracy ? *measurements.accuracy : qnan);
  }

  out.elite.push_back(std::move(run));
}

results_snapshot::results_snapshot(results_state &state)
  : current_fitness(state.collection.size()),
    reference_fitness(state.collection.size()),
    current_elite(state.collection.size()),
    reference_elite(state.collection.size())
{
  std::shared_lock guard(state.current_mutex);

  const std::size_t size(state.collection.size());
  labels.reserve(size);
  capacity_runs.reserve(size);
  runs.reserve(size * 2);
  success_rates.reserve(size * 2);
  success_thresholds.reserve(size);
  time_per_run.reserve(size * 2);

  references_available = std::ranges::any_of(
    state.collection,
    [](const auto &entry) { return !entry.second.reference.empty(); });

  for (const auto &[label, data] : state.collection)
  {
    labels.push_back(label);
    capacity_runs.push_back(data.conf.runs);
    const bool has_threshold(state.mode == rs::exec_mode::summary
                             || !data.conf.threshold.empty());
    success_thresholds.push_back(has_threshold);
    show_success_rate |= has_threshold;
    runs.push_back(data.current.runs);
    success_rates.push_back(data.current.empty()
                            ? std::numeric_limits<double>::quiet_NaN()
                            : data.current.success_rate * 100.0);
    time_per_run.push_back(data.current.empty()
                           ? std::numeric_limits<double>::quiet_NaN()
                           : std::chrono::duration<double>(
                               data.current.elapsed_time).count()
                             / data.current.runs);
    max_runs = std::max({max_runs, data.conf.runs, data.current.runs});
    add_fit_data(current_fitness, data.current);
    add_fit_data(reference_fitness, data.reference);
    add_elite_data(current_elite, data.current);
    add_elite_data(reference_elite, data.reference);
  }

  max_runs_with_reference = max_runs;
  for (const auto &[_, data] : state.collection)
  {
    runs.push_back(data.reference.runs);
    success_rates.push_back(data.reference.empty()
                            ? std::numeric_limits<double>::quiet_NaN()
                            : data.reference.success_rate * 100.0);
    time_per_run.push_back(data.reference.empty()
                           ? std::numeric_limits<double>::quiet_NaN()
                           : std::chrono::duration<double>(
                               data.reference.elapsed_time).count()
                             / data.reference.runs);
    max_runs_with_reference = std::max(max_runs_with_reference,
                                       data.reference.runs);
  }

  label_data = to_cstr_vector(labels);
  if (size)
    positions = make_positions(size);

  for (auto &run : current_elite.elite)
  {
    run.tick_text.reserve(run.id.size());
    std::ranges::transform(run.id, std::back_inserter(run.tick_text),
                           [](unsigned id) { return std::to_string(id); });
    run.tick_labels = to_cstr_vector(run.tick_text);
    if (!run.id.empty())
      run.tick_positions = make_positions(run.id.size());
  }
}

void render_number_of_runs(const results_snapshot &snapshot)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  constexpr double bar_width(0.5), half_width(bar_width / 2.0);

  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool show_reference_values {snapshot.references_available};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##Run##Runs", &show_reference_values);

  const std::size_t group_count(1 + show_reference_values);

  const unsigned max_runs(show_reference_values
                          ? snapshot.max_runs_with_reference
                          : snapshot.max_runs);

  int flags(ImPlotFlags_NoTitle);
  if (!snapshot.references_available || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Runs##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Runs");

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_X1, -padding, (size - 1) + padding);
  ImPlot::SetupAxisTicks(ImAxis_X1, snapshot.positions.data(), size,
                         snapshot.label_data.data());

  ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, max_runs + 1.0);

  // Bars.
  ImPlot::PlotBarGroups(ilabels.data(), snapshot.runs.data(), group_count, size,
                        bar_width);

  // Capacity lines.
  for (std::size_t i(0); i < size; ++i)
    if (snapshot.capacity_runs[i] > 1)
    {
      const auto di(static_cast<double>(i));
      const auto dcp(static_cast<double>(snapshot.capacity_runs[i]));

      const double xs[] =
      {
        di - half_width, show_reference_values ? di : di + half_width
      };

      const double ys[] = { dcp, dcp };

      ImPlot::PlotLine("##TotalRuns", xs, ys, 2,
                       {ImPlotProp_LineColor, ImVec4(1, 1, 0, 1),
                        ImPlotProp_LineWeight, 2.5f});
    }

  ImPlot::EndPlot();
}

[[nodiscard]] std::string success_rate_tooltip(
  const results_snapshot &snapshot, std::size_t i, bool show_reference_values)
{
  Expects(i < snapshot.labels.size());

  std::string text(snapshot.labels[i]);
  if (!snapshot.success_thresholds[i])
    return text + "\nNo success threshold defined";

  if (snapshot.runs[i])
    text += std::format("\nCurrent: {:.1f}% ({} completed runs)",
                        snapshot.success_rates[i], snapshot.runs[i]);
  else
    text += "\nCurrent: awaiting results";

  if (show_reference_values)
  {
    const auto reference(snapshot.labels.size() + i);
    if (snapshot.runs[reference])
      text += std::format("\nReference: {:.1f}% ({} completed runs)",
                          snapshot.success_rates[reference],
                          snapshot.runs[reference]);
    else
      text += "\nReference: no reference";

    if (snapshot.runs[i] && snapshot.runs[reference])
      text += std::format("\nDifference: {:+.1f} percentage points",
                          snapshot.success_rates[i]
                          - snapshot.success_rates[reference]);
  }

  return text;
}

void render_success_rate(const results_snapshot &snapshot)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool compare_reference {true};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##Run##Success rate",
                    &compare_reference);
  const bool show_reference_values(snapshot.references_available
                                   && compare_reference);
  const std::size_t group_count(1 + show_reference_values);

  int flags(ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText);
  if (!snapshot.references_available || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Success rate##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Success rate", 0, ImPlotAxisFlags_Lock);

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_X1, -padding, (size - 1) + padding);
  ImPlot::SetupAxisTicks(ImAxis_X1, snapshot.positions.data(), size,
                         snapshot.label_data.data());

  ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);
  ImPlot::SetupAxisFormat(ImAxis_Y1, "%.0f%%");

  const double bar_width(0.5 / group_count);
  for (std::size_t series(0); series < group_count; ++series)
  {
    std::vector<double> xs, ys;
    xs.reserve(size);
    ys.reserve(size);
    const double shift((series + 0.5) * bar_width - 0.25);
    for (std::size_t i(0); i < size; ++i)
    {
      const auto index(series * size + i);
      const double x(snapshot.positions[i] + shift);
      if (!snapshot.success_thresholds[i])
      {
        if (!series)
          ImPlot::PlotText("No threshold", snapshot.positions[i], 0.0,
                           ImVec2(0, -10));
      }
      else if (snapshot.runs[index])
      {
        xs.push_back(x);
        ys.push_back(snapshot.success_rates[index]);
      }
      else
        ImPlot::PlotText(series ? "N/A" : "Pending", x, 0.0,
                         ImVec2(0, series ? -26 : -10));
    }
    ImPlot::PlotBars(ilabels[series], xs.data(), ys.data(), xs.size(), bar_width);
  }

  if (ImPlot::IsPlotHovered())
  {
    const double x(ImPlot::GetPlotMousePos().x);
    if (x >= -0.5 && x < size - 0.5)
    {
      const auto i(static_cast<std::size_t>(std::floor(x + 0.5)));
      ImGui::BeginTooltip();
      const auto text(success_rate_tooltip(snapshot, i, show_reference_values));
      ImGui::TextUnformatted(text.c_str());
      ImGui::EndTooltip();
    }
  }

  ImPlot::EndPlot();
}

[[nodiscard]] std::optional<double> estimated_time_remaining(
  const results_snapshot &snapshot, std::size_t i)
{
  Expects(i < snapshot.labels.size());

  if (snapshot.runs[i] >= snapshot.capacity_runs[i])
    return 0.0;
  if (!snapshot.runs[i])
    return std::nullopt;

  return snapshot.time_per_run[i]
         * (snapshot.capacity_runs[i] - snapshot.runs[i]);
}

[[nodiscard]] std::string remaining_time_label(
  const results_snapshot &snapshot, std::size_t i)
{
  const auto estimate(estimated_time_remaining(snapshot, i));
  if (!estimate)
    return "Pending";
  if (snapshot.runs[i] >= snapshot.capacity_runs[i])
    return "Complete";

  using namespace std::chrono;
  const auto total(ceil<seconds>(duration<double>(*estimate)));
  const auto h(duration_cast<hours>(total));
  const auto m(duration_cast<minutes>(total - h));
  const auto s(total - h - m);
  if (h.count())
    return std::format("~{}h {}m {}s", h.count(), m.count(), s.count());
  if (m.count())
    return std::format("~{}m {}s", m.count(), s.count());
  return std::format("~{}s", s.count());
}

[[nodiscard]] std::string time_per_run_tooltip(
  const results_snapshot &snapshot, std::size_t i, bool show_reference_values)
{
  Expects(i < snapshot.labels.size());

  std::string text(snapshot.labels[i]);
  if (snapshot.runs[i])
    text += std::format("\nCurrent: {:.3f} s/run ({} completed runs)",
                        snapshot.time_per_run[i], snapshot.runs[i]);
  else
    text += "\nCurrent: awaiting results";

  const unsigned remaining(std::max(snapshot.capacity_runs[i], snapshot.runs[i])
                           - snapshot.runs[i]);
  text += std::format("\nRuns left: {}\nEstimated time remaining: {}",
                      remaining, remaining_time_label(snapshot, i));

  if (show_reference_values)
  {
    const auto reference(snapshot.labels.size() + i);
    if (snapshot.runs[reference])
      text += std::format("\nReference: {:.3f} s/run ({} completed runs)",
                          snapshot.time_per_run[reference],
                          snapshot.runs[reference]);
    else
      text += "\nReference: no reference";
  }

  return text;
}

void render_time_per_run(const results_snapshot &snapshot)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool compare_reference {true};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##Run##Time per run", &compare_reference);
  const bool show_reference_values(snapshot.references_available
                                   && compare_reference);
  const std::size_t group_count(1 + show_reference_values);

  std::vector<std::string> remaining_labels;
  remaining_labels.reserve(size);
  for (std::size_t i(0); i < size; ++i)
    remaining_labels.push_back(remaining_time_label(snapshot, i));
  const auto remaining_label_data(to_cstr_vector(remaining_labels));

  ImGui::TextDisabled("Remaining times update after each completed run.");

  int flags(ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText);
  if (!show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Time per run##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  const bool has_timing_data(std::ranges::any_of(
    snapshot.runs | std::views::take(size * group_count),
    [](unsigned runs) { return runs > 0; }));
  ImPlot::SetupAxes("Time per run (s)", "Dataset",
                    has_timing_data ? ImPlotAxisFlags_AutoFit
                                    : ImPlotAxisFlags_Lock,
                    ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert);
  if (!has_timing_data)
    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 1.0, ImGuiCond_Always);
  ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0,
                                    std::numeric_limits<double>::infinity());
  ImPlot::SetupAxisFormat(ImAxis_X1, "%.3f");

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_Y1, -padding, (size - 1) + padding,
                          ImGuiCond_Always);
  ImPlot::SetupAxisTicks(ImAxis_Y1, snapshot.positions.data(), size,
                         snapshot.label_data.data());
  ImPlot::SetupAxis(ImAxis_Y2, "Estimated time remaining",
                    ImPlotAxisFlags_Lock | ImPlotAxisFlags_Invert
                    | ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines
                    | ImPlotAxisFlags_NoTickMarks);
  ImPlot::SetupAxisLimits(ImAxis_Y2, -padding, (size - 1) + padding,
                          ImGuiCond_Always);
  ImPlot::SetupAxisTicks(ImAxis_Y2, snapshot.positions.data(), size,
                         remaining_label_data.data(), false);

  const double bar_width(0.5 / group_count);
  for (std::size_t series(0); series < group_count; ++series)
  {
    std::vector<double> xs, ys;
    const double shift((series + 0.5) * bar_width - 0.25);
    for (std::size_t i(0); i < size; ++i)
    {
      const auto index(series * size + i);
      const double y(snapshot.positions[i] + shift);
      if (snapshot.runs[index])
      {
        xs.push_back(snapshot.time_per_run[index]);
        ys.push_back(y);
      }
      else
        ImPlot::PlotText(series ? "N/A" : "Pending", 0.0, y,
                         ImVec2(35, 0),
                         {ImPlotProp_Flags, ImPlotItemFlags_NoFit});
    }
    ImPlot::PlotBars(ilabels[series], xs.data(), ys.data(), xs.size(), bar_width,
                     {ImPlotProp_Flags, ImPlotBarsFlags_Horizontal});
  }

  if (ImPlot::IsPlotHovered())
  {
    const double y(ImPlot::GetPlotMousePos().y);
    if (y >= -0.5 && y < size - 0.5)
    {
      const auto i(static_cast<std::size_t>(std::floor(y + 0.5)));
      ImGui::BeginTooltip();
      const auto text(time_per_run_tooltip(snapshot, i, show_reference_values));
      ImGui::TextUnformatted(text.c_str());
      ImGui::EndTooltip();
    }
  }

  ImPlot::EndPlot();
}

void render_fitness_series(const char *label, const double x, const double mean,
                           const double std_dev, const double best)
{
  ImPlot::PlotErrorBars(label, &x, &mean, &std_dev, 1);
  ImPlot::PlotScatter(label, &x, &mean, 1,
                      {ImPlotProp_Marker, ImPlotMarker_Circle});
  ImPlot::PlotScatter(label, &x, &best, 1,
                      {ImPlotProp_Marker, ImPlotMarker_Up});
}

void render_fitness_across_datasets(const results_snapshot &snapshot)
{
  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool show_reference_values {snapshot.references_available};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##FAD", &show_reference_values);

  const auto &current(snapshot.current_fitness);
  const auto &reference(snapshot.reference_fitness);
  const auto &labels(snapshot.label_data);
  static const char title[] = "##FAD";
  const auto n(static_cast<std::size_t>(std::ceil(std::sqrt(size))));

  if (!ImPlot::BeginSubplots(title, n, n, ImVec2(-1, -1),
                             ImPlotSubplotFlags_NoLegend))
    return;

  for (std::size_t i(0); i < size; ++i)
  {
    bool style_pushed(false);

    if (show_reference_values
        && std::isfinite(current.best[i]) && std::isfinite(reference.best[i])
        && current.best[i] < reference.best[i])
    {
      const float alpha(current.mean[i] < reference.mean[i] ? 0.4f : 0.2f);
      ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0f, 0.0f, 0.0f, alpha));
      style_pushed = true;
    }

    int flags(0);
    if (!snapshot.references_available || !show_reference_values)
      flags |= ImPlotFlags_NoLegend;

    if (ImPlot::BeginPlot(labels[i], ImVec2(-1, -1), flags))
    {
      ImPlot::SetupAxes("Runs", "Fitness", 0, ImPlotAxisFlags_AutoFit);

      const double min_x(reference.runs[i] > 0.0 && show_reference_values
                         ? 0.95 * std::min(current.runs[i], reference.runs[i])
                         : 0.0);
      const double max_x(reference.runs[i] > 0.0 && show_reference_values
                         ? 1.05 * std::max(current.runs[i], reference.runs[i])
                         : 2.0 * std::max(current.runs[i], 1.0));
      //ImPlot::SetupAxesLimits(min_x, max_x, 0.0, 0.0);
      ImPlot::SetupAxisLimits(ImAxis_X1, min_x, max_x, ImGuiCond_Always);

      // Current data.
      render_fitness_series((current_str + "##FAD" + labels[i]).c_str(),
                             current.runs[i], current.mean[i],
                             current.std_dev[i], current.best[i]);

      // Reference data.
      if (show_reference_values)
      {
        const auto xs(
          reference.runs[i]
          + (ultra::almost_equal(current.runs[i],reference.runs[i])
             ? 0.1 : 0.0));
        render_fitness_series((reference_str + "##FAD" + labels[i]).c_str(),
                               xs, reference.mean[i], reference.std_dev[i],
                               reference.best[i]);
      }

      ImPlot::EndPlot();
    }

    if (style_pushed)
      ImPlot::PopStyleColor();
  }

  ImPlot::EndSubplots();
}

void render_accuracy_labels(const elite_data::run &series, double shift = 0.0)
{
  const ImVec2 offset_upward(0, -10);

  for (std::size_t k(0); k < series.fit.size(); ++k)
    if (std::isfinite(series.acc[k]))
    {
      const std::string text(std::format("{:.2f}%", series.acc[k]*100.0));
      ImPlot::PlotText(text.c_str(), static_cast<double>(k) + shift,
                       series.fit[k], offset_upward);
    }
}

void render_elite(const results_snapshot &snapshot)
{
  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool show_reference_values {snapshot.references_available};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##ELITE", &show_reference_values);

  const auto &current(snapshot.current_elite);
  const auto &reference(snapshot.reference_elite);
  const auto &labels(snapshot.label_data);
  static const char title[] = "##ELITE";
  const auto n(static_cast<std::size_t>(std::ceil(std::sqrt(size))));

  if (!ImPlot::BeginSubplots(title, n, n, ImVec2(-1, -1),
                             ImPlotSubplotFlags_NoLegend))
    return;

  for (std::size_t i(0); i < size; ++i)
  {
    int flags(0);
    if (!snapshot.references_available || !show_reference_values)
      flags |= ImPlotFlags_NoLegend;

    if (ImPlot::BeginPlot(labels[i], ImVec2(-1, -1), flags))
    {
      ImPlot::SetupAxes("Rank", "Fitness", 0, ImPlotAxisFlags_AutoFit);

      // `ImPlotAxisFlags_Opposite` is the magic flag that moves the axis to
      // the top.
      ImPlot::SetupAxis(ImAxis_X2, "Run", ImPlotAxisFlags_Opposite);

      const auto rank_count(std::max(current.elite[i].id.size(),
                                     reference.elite[i].id.size()));
      const double max_x(rank_count);

      std::vector<double> rank_positions;
      std::vector<std::string> rank_text;
      if (rank_count)
      {
        rank_positions = make_positions(rank_count);
        rank_text.reserve(rank_count);
        for (std::size_t rank(1); rank <= rank_count; ++rank)
          rank_text.push_back(std::to_string(rank));
      }
      const auto rank_labels(to_cstr_vector(rank_text));
      ImPlot::SetupAxisTicks(ImAxis_X1, rank_positions.data(),
                             static_cast<int>(rank_positions.size()),
                             rank_labels.data(), false);

      double x_from(-0.5), x_to(max_x + 0.5);
      ImPlot::SetupAxisLimits(ImAxis_X1, x_from, x_to, ImGuiCond_Always);
      ImPlot::SetupAxisLinks(ImAxis_X1, &x_from, &x_to);
      ImPlot::SetupAxisLinks(ImAxis_X2, &x_from, &x_to);

      if (const auto &ids(current.elite[i].id); !ids.empty())
        ImPlot::SetupAxisTicks(ImAxis_X2,
                               current.elite[i].tick_positions.data(),
                               static_cast<int>(ids.size()),
                               current.elite[i].tick_labels.data());

      // Current data.
      {
        const auto &fit(current.elite[i].fit);

        ImPlot::PlotBars((current_str + "##ELITE" + labels[i]).c_str(),
                         fit.data(), fit.size(), 0.4);

        render_accuracy_labels(current.elite[i]);
      }

      // Reference data.
      if (show_reference_values)
      {
        const auto &fit(reference.elite[i].fit);
        const double bar_shift(0.1);

        auto bar_col(ImPlot::GetColormapColor(1));
        bar_col.w *= 0.7f;

        ImPlot::PlotBars((reference_str + "##ELITE" + labels[i]).c_str(),
                          fit.data(), fit.size(), 0.4, bar_shift,
                          {ImPlotProp_FillColor, bar_col});

        auto text_col(ImGui::GetStyleColorVec4(ImGuiCol_Text));
        text_col.w *= 0.55f;
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);

        render_accuracy_labels(reference.elite[i], bar_shift);

        ImGui::PopStyleColor();
      }

      ImPlot::EndPlot();
    }
  }

  ImPlot::EndSubplots();
}

void render_rs(const imgui_app::program &prg, bool *p_open,
               results_state &state)
{
  const results_snapshot snapshot(state);

  const auto fa(prg.free_area());
  ImGui::SetNextWindowPos(ImVec2(fa.x, fa.y));
  ImGui::SetNextWindowSize(ImVec2(fa.w, fa.h));

  static bool show_runs_check(true);
  static bool show_success_rate_check(true);
  static bool show_fitness_across_datasets_check(true);
  static bool show_elite_check(true);

  static bool mxz_runs(false);
  static bool mxz_success_rate(false);
  static bool mxz_fitness_across_datasets(false);
  static bool mxz_elite(false);

  if (ImGui::Begin("Run##Window", p_open))
  {
    ImGui::Checkbox("runs", &show_runs_check);
    ImGui::SameLine();
    ImGui::Checkbox(snapshot.show_success_rate ? "success rate" : "time per run",
                    &show_success_rate_check);
    ImGui::SameLine();
    ImGui::Checkbox("fitness across datasets",
                    &show_fitness_across_datasets_check);
    ImGui::SameLine();
    ImGui::Checkbox("elite", &show_elite_check);
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    std::array panels
    {
      dashboard_panel
      {
        "Runs##ChildWindow", "RUNS", show_runs_check, mxz_runs,
        [&snapshot] { render_number_of_runs(snapshot); }
      },
      dashboard_panel
      {
        "Success rate##ChildWindow",
        snapshot.show_success_rate ? "SUCCESS RATE" : "TIME PER RUN",
        show_success_rate_check, mxz_success_rate,
        [&snapshot]
        {
          if (snapshot.show_success_rate)
            render_success_rate(snapshot);
          else
            render_time_per_run(snapshot);
        }
      },
      dashboard_panel
      {
        "FADs##ChildWindow", "FITNESS ACROSS DATASETS",
        show_fitness_across_datasets_check, mxz_fitness_across_datasets,
        [&snapshot] { render_fitness_across_datasets(snapshot); }
      },
      dashboard_panel
      {
        "ELITEs##ChildWindow", "ELITE RUNS", show_elite_check, mxz_elite,
        [&snapshot] { render_elite(snapshot); }
      }
    };
    render_dashboard(panels);
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();
}

std::vector<double> make_positions(std::size_t n)
{
  assert(n);

  return std::views::iota(0uz, n)
       | std::views::transform([](auto i) { return static_cast<double>(i); })
       | std::ranges::to<std::vector<double>>();
}

std::vector<const char *> to_cstr_vector(const std::vector<std::string> &v)
{
  return v | std::views::transform(&std::string::c_str)
           | std::ranges::to<std::vector>();
}

// Asynchronously reads all available summary files into queues for subsequent
// processing.
void get_summaries(std::stop_token stoken, results_state &state)
{
  assert(!state.collection.empty());

  while (!stoken.stop_requested())
  {
    for (auto &test : state.collection)
    {
      const auto xml_fn(test.second.xml_summary);

      if (tinyxml2::XMLDocument summary; summary_data::check(xml_fn, summary))
      {
        std::lock_guard guard(state.current_mutex);
        test.second.current = summary_data(summary);
      }
    }

    std::this_thread::sleep_for(3000ms);
  }
}
void rs::summary::start(const imgui_app::program::settings &settings,
                        options options)
{
  results_state state(std::move(options.collection), options.mode);
  std::jthread t_summaries(get_summaries, std::ref(state));

  imgui_app::program prg(settings);
  prg.run(
    [&state](const auto &program, bool *open)
    {
      render_rs(program, open, state);
    });
}

}  // namespace ultra::wopr
