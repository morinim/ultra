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
  explicit results_state(rs::collection_t c) : collection(std::move(c)) {}

  rs::collection_t collection;
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
  std::vector<double> success_rates;
  double best_success_rate {0.0};
  double best_success_rate_with_reference {0.0};
  fit_data current_fitness;
  fit_data reference_fitness;
  elite_data current_elite;
  elite_data reference_elite;
};

bool summary_data::check(const fs::path &xml_fn, tinyxml2::XMLDocument &doc)
{
  if (doc.LoadFile(xml_fn.c_str()) != tinyxml2::XML_SUCCESS
      || !doc.FirstChild())
    return false;

  std::ifstream in(xml_fn, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string xml_content(ss.str());

  return ultra::crc32::verify_xml_signature(xml_content);
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

  references_available = std::ranges::any_of(
    state.collection,
    [](const auto &entry) { return !entry.second.reference.empty(); });

  for (const auto &[label, data] : state.collection)
  {
    labels.push_back(label);
    capacity_runs.push_back(data.conf.runs);
    runs.push_back(data.current.runs);
    success_rates.push_back(data.current.success_rate * 100.0);
    max_runs = std::max({max_runs, data.conf.runs, data.current.runs});
    best_success_rate = std::max(best_success_rate,
                                 data.current.success_rate);
    add_fit_data(current_fitness, data.current);
    add_fit_data(reference_fitness, data.reference);
    add_elite_data(current_elite, data.current);
    add_elite_data(reference_elite, data.reference);
  }

  max_runs_with_reference = max_runs;
  best_success_rate_with_reference = best_success_rate;
  for (const auto &[_, data] : state.collection)
  {
    runs.push_back(data.reference.runs);
    success_rates.push_back(data.reference.success_rate * 100.0);
    max_runs_with_reference = std::max(max_runs_with_reference,
                                       data.reference.runs);
    best_success_rate_with_reference = std::max(
      best_success_rate_with_reference, data.reference.success_rate);
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

      ImPlot::SetNextLineStyle(ImVec4(1, 1, 0, 1), 2.5f); // yellow, thick
      ImPlot::PlotLine("##TotalRuns", xs, ys, 2);
    }

  ImPlot::EndPlot();
}

void render_success_rate(const results_snapshot &snapshot)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  const std::size_t size(snapshot.labels.size());
  if (!size)
    return;

  static bool show_reference_values {snapshot.references_available};
  if (snapshot.references_available)
    ImGui::Checkbox("Reference values##Run##Success rate",
                    &show_reference_values);
  const std::size_t group_count(1 + show_reference_values);

  const double best_success_rate(
    show_reference_values ? snapshot.best_success_rate_with_reference
                          : snapshot.best_success_rate);

  int flags(ImPlotFlags_NoTitle);
  if (!snapshot.references_available || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Success rate##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Success rate");

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_X1, -padding, (size - 1) + padding);
  ImPlot::SetupAxisTicks(ImAxis_X1, snapshot.positions.data(), size,
                         snapshot.label_data.data());

  if (best_success_rate > 0.0)
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0,
                            std::min(100.0, 100.0 * best_success_rate + 5.0),
                            ImGuiCond_Always);
  else
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);

  ImPlot::PlotBarGroups(ilabels.data(), snapshot.success_rates.data(),
                        group_count, size, 0.5);
  ImPlot::EndPlot();
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
      if (current.mean[i] < reference.mean[i])
      {
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0, 0.0, 0.0, 0.4));
        style_pushed = true;
      }
      else
      {
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0, 0.0, 0.0, 0.2));
        style_pushed = true;
      }
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
      {
        const auto xs(current.runs[i]);
        const auto ys(current.mean[i]);
        const auto ys_dev(current.std_dev[i]);
        const auto ys_best(current.best[i]);

        ImPlot::PlotErrorBars((current_str + "##FAD" + labels[i]).c_str(),
                              &xs, &ys, &ys_dev, 1);
        ImPlot::PlotScatter((current_str + "##FAD" + labels[i]).c_str(),
                            &xs, &ys, 1);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up);
        ImPlot::PlotScatter((current_str + "##FAD" + labels[i]).c_str(),
                            &xs, &ys_best, 1);
      }

      // Reference data.
      if (show_reference_values)
      {
        const auto xs(
          reference.runs[i]
          + (ultra::almost_equal(current.runs[i],reference.runs[i])
             ? 0.1 : 0.0));
        const auto ys(reference.mean[i]);
        const auto ys_dev(reference.std_dev[i]);
        const auto ys_best(reference.best[i]);

        ImPlot::PlotErrorBars((reference_str + "##FAD" + labels[i]).c_str(),
                              &xs, &ys, &ys_dev, 1);
        ImPlot::PlotScatter((reference_str + "##FAD" + labels[i]).c_str(),
                            &xs, &ys, 1);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up);
        ImPlot::PlotScatter((reference_str + "##FAD" + labels[i]).c_str(),
                            &xs, &ys_best, 1);
      }

      ImPlot::EndPlot();
    }

    if (style_pushed)
      ImPlot::PopStyleColor();
  }

  ImPlot::EndSubplots();
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

      const double max_x(std::max(current.elite[i].id.size(),
                                  reference.elite[i].id.size()));

      double x_from(-0.5), x_to(max_x + 0.5);
      ImPlot::SetupAxisLimits(ImAxis_X1, x_from, x_to, ImGuiCond_Always);
      ImPlot::SetupAxisLinks(ImAxis_X1, &x_from, &x_to);
      ImPlot::SetupAxisLinks(ImAxis_X2, &x_from, &x_to);

      if (const auto &ids(current.elite[i].id); !ids.empty())
      {
        ImPlot::SetupAxisTicks(ImAxis_X2,
                               current.elite[i].tick_positions.data(),
                               static_cast<int>(ids.size()),
                               current.elite[i].tick_labels.data());
      }

      const ImVec2 offset_upward(0, -10);

      // Current data.
      {
        const auto &fit(current.elite[i].fit);
        const auto &acc(current.elite[i].acc);

        ImPlot::PlotBars((current_str + "##ELITE" + labels[i]).c_str(),
                         fit.data(), fit.size(), 0.4);

        // Accuracy labels.
        for (std::size_t k(0); k < fit.size(); ++k)
          if (std::isfinite(acc[k]))
          {
            const std::string text(std::format("{:.2f}%", acc[k]*100.0));
            ImPlot::PlotText(text.c_str(), static_cast<double>(k),
                             fit[k], offset_upward);
          }
      }

      // Reference data.
      if (show_reference_values)
      {
        const auto &fit(reference.elite[i].fit);
        const auto &acc(reference.elite[i].acc);
        const double bar_shift(0.1);

        auto bar_col(ImPlot::GetColormapColor(1));
        bar_col.w *= 0.7f;
        ImPlot::PushStyleColor(ImPlotCol_Fill, bar_col);

        ImPlot::PlotBars((reference_str + "##ELITE" + labels[i]).c_str(),
                          fit.data(), fit.size(), 0.4, bar_shift);

        ImPlot::PopStyleColor();

        auto text_col(ImGui::GetStyleColorVec4(ImGuiCol_Text));
        text_col.w *= 0.55f;
        ImGui::PushStyleColor(ImGuiCol_Text, text_col);

        for (std::size_t k(0); k < fit.size(); ++k)
          if (std::isfinite(acc[k]))
          {
            const std::string text(std::format("{:.2f}%", acc[k]*100.0));
            ImPlot::PlotText(text.c_str(),
                             static_cast<double>(k) + bar_shift,
                             fit[k],
                             offset_upward);
          }

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
    ImGui::Checkbox("success rate", &show_success_rate_check);
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
        "Success rate##ChildWindow", "SUCCESS RATE", show_success_rate_check,
        mxz_success_rate, [&snapshot] { render_success_rate(snapshot); }
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
  results_state state(std::move(options.collection));
  std::jthread t_summaries(get_summaries, std::ref(state));

  imgui_app::program prg(settings);
  prg.run(
    [&state](const auto &program, bool *open)
    {
      render_rs(program, open, state);
    });
}

}  // namespace ultra::wopr
