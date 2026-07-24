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

[[nodiscard]] labels_data make_labels(const rs::collection_t &);
[[nodiscard]] std::vector<double> make_positions(std::size_t);
[[nodiscard]] std::vector<const char *> to_cstr_vector(
  const std::vector<std::string> &);

[[nodiscard]] bool references_available(const results_state &state) noexcept
{
  return std::ranges::any_of(
    state.collection,
    [](const auto &entry) { return !entry.second.reference.empty(); });
}

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
void render_number_of_runs(results_state &state)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  constexpr double bar_width(0.5), half_width(bar_width / 2.0);

  const std::size_t size(state.collection.size());
  if (!size)
    return;

  static bool show_reference_values {references_available(state)};
  if (references_available(state))
    ImGui::Checkbox("Reference values##Run##Runs", &show_reference_values);

  const std::size_t group_count(1 + show_reference_values);

  struct cap_data
  {
    std::vector<unsigned> runs;
    unsigned max;
  };

  const cap_data caps = [&]
  {
    cap_data out;
    out.max = 0;
    out.runs.reserve(size);

    for (const auto &[_, data] : state.collection)
    {
      out.runs.push_back(data.conf.runs);
      out.max = std::max(out.max, data.conf.runs);
    }

    return out;
  }();

  std::vector<unsigned> bar_values;
  bar_values.reserve(size * group_count);

  unsigned max_runs(caps.max);

  for (std::shared_lock guard(state.current_mutex);
       const auto &[_, data] : state.collection)
  {
    bar_values.push_back(data.current.runs);
    max_runs = std::max(max_runs, data.current.runs);
  }

  if (show_reference_values)
    for (const auto &[_, data] : state.collection)
    {
      bar_values.push_back(data.reference.runs);
      max_runs = std::max(max_runs, data.reference.runs);
    }

  const auto labels(make_labels(state.collection));
  const auto positions(make_positions(size));

  int flags(ImPlotFlags_NoTitle);
  if (!references_available(state) || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Runs##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Runs");

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_X1, -padding, (size - 1) + padding);
  ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), size, labels.data());

  ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, max_runs + 1.0);

  // Bars.
  ImPlot::PlotBarGroups(ilabels.data(), bar_values.data(), group_count, size,
                        bar_width);

  // Capacity lines.
  for (std::size_t i(0); i < size; ++i)
    if (caps.runs[i] > 1)
    {
      const auto di(static_cast<double>(i));
      const auto dcp(static_cast<double>(caps.runs[i]));

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

void render_success_rate(results_state &state)
{
  static const std::vector ilabels {current_str.c_str(), reference_str.c_str()};

  const std::size_t size(state.collection.size());
  if (!size)
    return;

  static bool show_reference_values {references_available(state)};
  if (references_available(state))
    ImGui::Checkbox("Reference values##Run##Success rate",
                    &show_reference_values);
  const std::size_t group_count(1 + show_reference_values);

  std::vector<double> sr;
  sr.reserve(size * group_count);

  double best_success_rate(0.0);
  for (std::shared_lock guard(state.current_mutex);
       const auto &[_, data] : state.collection)
  {
    best_success_rate = std::max(data.current.success_rate, best_success_rate);
    sr.push_back(data.current.success_rate * 100.0);
  }

  if (show_reference_values)
    for (const auto &[_, data] : state.collection)
    {
      best_success_rate = std::max(data.reference.success_rate,
                                   best_success_rate);
      sr.push_back(data.reference.success_rate * 100.0);
    }

  const auto labels(make_labels(state.collection));
  const auto positions(make_positions(size));

  int flags(ImPlotFlags_NoTitle);
  if (!references_available(state) || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Success rate##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Success rate");

  constexpr auto padding(0.75);
  ImPlot::SetupAxisLimits(ImAxis_X1, -padding, (size - 1) + padding);
  ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), size, labels.data());

  if (best_success_rate > 0.0)
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0,
                            std::min(100.0, 100.0 * best_success_rate + 5.0),
                            ImGuiCond_Always);
  else
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);

  ImPlot::PlotBarGroups(ilabels.data(), sr.data(), group_count, size, 0.5);
  ImPlot::EndPlot();
}

void render_fitness_across_datasets(results_state &state)
{
  const std::size_t size(state.collection.size());
  if (!size)
    return;

  static bool show_reference_values {references_available(state)};
  if (references_available(state))
    ImGui::Checkbox("Reference values##FAD", &show_reference_values);

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

  const auto add_data([](fit_data &fitd, const summary_data &sumd)
  {
    constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

    if (sumd.best_fit.size())
      fitd.best.push_back(sumd.best_fit[0]);
    else
      fitd.best.push_back(qnan);

    if (sumd.fit_mean.size())
      fitd.mean.push_back(sumd.fit_mean[0]);
    else
      fitd.mean.push_back(qnan);

    if (sumd.fit_std_dev.size())
      fitd.std_dev.push_back(sumd.fit_std_dev[0]);
    else
      fitd.std_dev.push_back(qnan);

    fitd.runs.push_back(sumd.runs);
  });

  fit_data current(size);

  for (std::shared_lock guard(state.current_mutex);
       const auto &[_, data] : state.collection)
    add_data(current, data.current);

  const fit_data reference = [&]
  {
    fit_data out(size);

    for (const auto &[_, data] : state.collection)
      add_data(out, data.reference);

    return out;
  }();

  const auto labels(make_labels(state.collection));
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
    if (!references_available(state) || !show_reference_values)
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

void render_elite(results_state &state)
{
  const std::size_t size(state.collection.size());
  if (!size)
    return;

  static bool show_reference_values {references_available(state)};
  if (references_available(state))
    ImGui::Checkbox("Reference values##ELITE", &show_reference_values);

  struct elite_data
  {
    explicit elite_data(std::size_t count) { elite.reserve(count); }

    struct run
    {
      std::vector<unsigned> id {};
      std::vector<double> fit {};
      std::vector<double> acc {};
    };

    std::vector<run> elite;
  };

  const auto add_data([](elite_data &e, const summary_data &sumd)
  {
    constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

    elite_data::run d;

    for (std::size_t i(0); i < sumd.elite.size(); ++i)
    {
      d.id.push_back(sumd.elite[i].first);

      if (sumd.elite[i].second.fitness)
        d.fit.push_back((*sumd.elite[i].second.fitness)[0]);
      else
        d.fit.push_back(qnan);

      if (sumd.elite[i].second.accuracy)
        d.acc.push_back(*sumd.elite[i].second.accuracy);
      else
        d.acc.push_back(qnan);
    }

    e.elite.push_back(d);
  });

  elite_data current(size);

  for (std::shared_lock guard(state.current_mutex);
       const auto &[_, data] : state.collection)
    add_data(current, data.current);

  const elite_data reference = [&]
  {
    elite_data out(size);

    for (const auto &[_, data] : state.collection)
      add_data(out, data.reference);

    return out;
  }();

  const auto labels(make_labels(state.collection));
  static const char title[] = "##ELITE";
  const auto n(static_cast<std::size_t>(std::ceil(std::sqrt(size))));

  if (!ImPlot::BeginSubplots(title, n, n, ImVec2(-1, -1),
                             ImPlotSubplotFlags_NoLegend))
    return;

  for (std::size_t i(0); i < size; ++i)
  {
    int flags(0);
    if (!references_available(state) || !show_reference_values)
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
        static std::vector<std::string> tick_text;
        tick_text.resize(ids.size());

        for (std::size_t k(0); k < ids.size(); ++k)
          tick_text[k] = std::to_string(ids[k]);

        const auto tick_cstr(to_cstr_vector(tick_text));
        const auto tick_pos(make_positions(current.elite[i].id.size()));

        ImPlot::SetupAxisTicks(ImAxis_X2,
                               tick_pos.data(),
                               static_cast<int>(tick_pos.size()),
                               tick_cstr.data());
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
        [&state] { render_number_of_runs(state); }
      },
      dashboard_panel
      {
        "Success rate##ChildWindow", "SUCCESS RATE", show_success_rate_check,
        mxz_success_rate, [&state] { render_success_rate(state); }
      },
      dashboard_panel
      {
        "FADs##ChildWindow", "FITNESS ACROSS DATASETS",
        show_fitness_across_datasets_check, mxz_fitness_across_datasets,
        [&state] { render_fitness_across_datasets(state); }
      },
      dashboard_panel
      {
        "ELITEs##ChildWindow", "ELITE RUNS", show_elite_check, mxz_elite,
        [&state] { render_elite(state); }
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

labels_data make_labels(const rs::collection_t &c)
{
  return c | std::views::keys
           | std::views::transform(&std::string::c_str)
           | std::ranges::to<labels_data>();
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
