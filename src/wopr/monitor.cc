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

#include "monitor.h"

#include "gui_helpers.h"
#include "monitor_data.h"

#include "kernel/gp/primitive/real.h"
#include "utility/timer.h"
#include "utility/ts_queue.h"

#include "implot/implot.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

using namespace std::chrono_literals;

ultra::ts_queue<dynamic_data> dynamic_queue;
ultra::ts_queue<population_line> population_queue;
ultra::ts_queue<layers_line> layers_queue;

namespace monitor
{

ultra::search_log slog {};
int window {0};

}  // namespace monitor

/*********************************************************************
 * A normal distribution that supports works when standard deviation is zero.
 ********************************************************************/
template<class RealType = double>
class degenerate_normal_distribution
{
public:
  using result_type = RealType;

  struct param_type
  {
    RealType mean {0.0};
    RealType stddev {1.0};

    param_type(RealType m, RealType sd) : mean(m), stddev(sd) {}
  };

  degenerate_normal_distribution(RealType mean, RealType stddev)
    : params_(mean, stddev),
      dist_(mean, ultra::issmall(stddev) ? RealType(1) : stddev)
  {}

  void reset() { dist_.reset(); }

  template<class URNG>
  [[nodiscard]] result_type operator()(URNG &g)
  {
    return ultra::issmall(params_.stddev) ? params_.mean : dist_(g);
  }

private:
  param_type params_;
  std::normal_distribution<RealType> dist_;
};
struct id_scope
{
  explicit id_scope(int id) { ImGui::PushID(id); }
  explicit id_scope(const void *p) { ImGui::PushID(p); }
  explicit id_scope(const char *p) { ImGui::PushID(p); }

  ~id_scope() { ImGui::PopID(); }

  id_scope(const id_scope &) = delete;
  id_scope &operator=(const id_scope &) = delete;
};

// Invariants:
// - `rs::collection` size and ordering are immutable after initialisation;
void render_dynamic()
{
  static std::vector<dynamic_sequence> dynamic_runs;

  while (const auto data = dynamic_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (dynamic_runs.empty() || !dynamic_runs.back().empty())
        dynamic_runs.push_back({});
    }
    else
    {
      if (dynamic_runs.empty())  // just to land on one's feet
        dynamic_runs.push_back({});

      dynamic_runs.back().push_back(*data);
    }
  }

  static bool show_best(true);
  static bool show_longest(true);

  const float avail(ImGui::GetContentRegionAvail().y * 0.85f);

  for (std::size_t run(dynamic_runs.size()); run--;)
  {
    auto &dr(dynamic_runs[run]);
    if (dr.empty())
      continue;

    id_scope scope(run);

    ImGui::SetNextItemOpen(run + 1 == dynamic_runs.size(), ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(("Run " + std::to_string(run)).c_str()))
      continue;

    if (ImGui::BeginTabBar("DynamicTabBar"))
    {
      const auto &xs(dr.xs);

      std::size_t window(monitor::window
                         ? static_cast<std::size_t>(monitor::window)
                         : std::numeric_limits<std::size_t>::max());
      window = std::min<std::size_t>(xs.size(), window);

      const auto get_window([&window](const auto &vect)
      {
        return vect.data() + vect.size() - window;
      });

      if (ImGui::BeginTabItem("Fitness dynamic"))
      {
        if (dr.best_prg.empty())
          dr.best_idx = 0;
        else
        {
          std::string best_prg;
          for (std::size_t i(dr.best_prg.size()); i; --i)
            best_prg += dr.best_prg[i - 1] + std::string(1, '\0');

          dr.best_idx = std::clamp(
            dr.best_idx, 0, static_cast<int>(dr.best_prg.size() - 1));
          ImGui::Combo("Best programs", &dr.best_idx, best_prg.data());
        }

        ImGui::SameLine();
        ImGui::Checkbox("Best##show", &show_best);

        if (ImPlot::BeginPlot("##Fitness by generation", ImVec2(-1, avail),
                              ImPlotFlags_NoTitle))
        {
          ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

          ImPlot::SetupAxes("Generation", "Fit",
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

          ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
          constexpr const char *avg_stddev("Avg & StdDev");
          ImPlot::PlotErrorBars(avg_stddev, get_window(xs),
                                get_window(dr.fit_mean),
                                get_window(dr.fit_std_dev), window);
          ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
          ImPlot::PlotLine(avg_stddev, get_window(xs), get_window(dr.fit_mean),
                           window);

          if (show_best)
          {
            ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
            ImPlot::PlotLine("Best", get_window(xs), get_window(dr.fit_best),
                             window);
          }

          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Length dynamic"))
      {
        ImGui::Checkbox("Longest##show", &show_longest);

        if (ImPlot::BeginPlot("##Length by generation", ImVec2(-1, avail),
                              ImPlotFlags_NoTitle))
        {
          ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

          ImPlot::SetupAxes(
            "Generation", "Length",
            ImPlotAxisFlags_AutoFit,  // ImPlotAxisFlags_None
            ImPlotAxisFlags_AutoFit);

          constexpr const char *avg_stddev("Len Avg & StdDev");
          ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
          ImPlot::PlotErrorBars(avg_stddev, get_window(xs),
                                get_window(dr.len_mean),
                                get_window(dr.len_std_dev), window);
          ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
          ImPlot::PlotLine(avg_stddev, get_window(xs), get_window(dr.len_mean),
                           window);

          if (show_longest)
          {
            ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
            ImPlot::PlotLine("Longest", get_window(xs), get_window(dr.len_max),
                             window);
          }

          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Crossover dynamics"))
      {
        if (ImPlot::BeginPlot("##Crossover types by generation",
                              ImVec2(-1, avail), ImPlotFlags_NoTitle))
        {
          ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

          ImPlot::SetupAxes(
            "Generation", "Crossover spreading",
            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

          // ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
          for (const auto &[ct, nums] : dr.crossover_types)
          {
            const std::string type("CT" + std::to_string(ct));
            ImPlot::PlotLine(type.c_str(), get_window(xs), get_window(nums),
                             window);
          }

          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
}

void render_population()
{
  static std::vector<population_sequence> population_runs;

  while (auto data = population_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (population_runs.empty() || !population_runs.back().empty())
        population_runs.push_back({});
    }
    else
    {
      if (population_runs.empty())  // just to land on one's feet
        population_runs.push_back({});

      population_runs.back().update(*data);
    }
  }

  const float avail(ImGui::GetContentRegionAvail().y * 0.85f);

  for (std::size_t run(population_runs.size()); run--;)
  {
    const auto &pr(population_runs[run]);
    if (pr.empty())
      continue;

    id_scope scope(run);

    ImGui::SetNextItemOpen(run + 1 == population_runs.size(), ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(("Run " + std::to_string(run)).c_str()))
      continue;

    if (ImGui::BeginTabBar("PopulationTabBar"))
    {
      if (ImGui::BeginTabItem("Fitness histogram"))
      {
        const std::string title("Generation " + std::to_string(pr.generation)
                                + "##Population");

        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, avail),
                              ImPlotFlags_NoLegend))
        {
          ImPlot::SetupAxes("Fitness", "Individuals",
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
          ImPlot::PlotBars("##PopulationFitnessHistogram",
                           pr.fit.data(), pr.obs.data(), pr.fit.size(), 0.8);
          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Fitness entropy"))
      {
        const std::string title("Generation " + std::to_string(pr.generation)
                                + "##Entropy");
        if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, avail),
                              ImPlotFlags_NoLegend))
        {
          ImPlot::SetupAxes("Generation", "Entropy",
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

          ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
          ImPlot::PlotShaded("Entropy", pr.xs.data(), pr.fit_entropy.data(),
                             pr.xs.size(),
                             -std::numeric_limits<double>::infinity());
          ImPlot::PlotLine("Entropy", pr.xs.data(), pr.fit_entropy.data(),
                           pr.xs.size());
          ImPlot::PopStyleVar();

          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
}

void render_layers_fit(const std::vector<layers_sequence> &layers_runs)
{
  static std::minstd_rand g;

  const float avail(ImGui::GetContentRegionAvail().y * 0.90f);

  for (std::size_t run(layers_runs.size()); run--;)
  {
    const auto &lr(layers_runs[run]);

    if (lr.empty())
      continue;

    id_scope scope(run);

    ImGui::SetNextItemOpen(run + 1 == layers_runs.size(), ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(("Run " + std::to_string(run)).c_str()))
      continue;

    const auto max_layers(lr.size());

    const std::size_t ind_max(std::ranges::max(lr.individuals));
    const std::size_t parts(std::min<std::size_t>(ind_max, 100));

    std::vector<double> fit;
    fit.resize(max_layers * parts);

    g.seed(lr.generation);

    const double fit_max(std::ranges::max(lr.fit_max));
    const double fit_min(std::ranges::min(lr.fit_min));

    std::vector<std::string> x_labels(parts);
    x_labels.front() = std::to_string(ind_max);
    x_labels.back() = "1";

    std::vector<const char *> x_labels_chr(parts);
    std::ranges::transform(x_labels, x_labels_chr.begin(),
                           [](const auto &str) { return str.data(); });

    std::vector<std::string> y_labels(max_layers);
    std::ranges::generate(y_labels,
                          [layer = 0u]() mutable
                          { return "L" + std::to_string(layer++); });

    std::vector<const char *> y_labels_chr(max_layers);
    std::ranges::transform(y_labels, y_labels_chr.begin(),
                           [](const auto &str) { return str.data(); });

    for (std::size_t layer(0); layer < max_layers; ++layer)
    {
      if (layer < lr.size())
      {
        degenerate_normal_distribution fit_nd(lr.fit_mean[layer],
                                              lr.fit_std_dev[layer]);

        const std::size_t full(parts * lr.individuals[layer] / ind_max);

        for (std::size_t p(0); p < full; ++p)
        {
          const auto rv(fit_nd(g));
          const auto pos(layer * parts + p);

          fit[pos] = std::clamp(rv, lr.fit_min[layer], lr.fit_max[layer]);
        }

        for (std::size_t p(full); p < parts; ++p)
          fit[layer * parts + p] = fit_min;
      }
      else
      {
        for (std::size_t p(0); p < parts; ++p)
          fit[layer * parts + p] = fit_min;
      }
    }

    static ImPlotColormap map(ImPlotColormap_Hot);
    ImPlot::PushColormap(map);

    const std::string title("Fitness by layer - Generation "
                            + std::to_string(lr.generation));

    ImPlot::ColormapScale("Fit Scale", fit_min, fit_max, ImVec2(80, avail));
    ImGui::SameLine();
    if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, avail),
                          ImPlotFlags_NoLegend|ImPlotFlags_NoMouseText))
    {
      ImPlot::SetupAxes(
        nullptr, nullptr,
        ImPlotAxisFlags_Lock|ImPlotAxisFlags_NoTickMarks,
        ImPlotAxisFlags_Lock|ImPlotAxisFlags_NoTickMarks
        |ImPlotAxisFlags_NoGridLines);

      ImPlot::SetupAxisTicks(ImAxis_Y1, 1 - 0.5/max_layers, 0.5/max_layers,
                             max_layers, y_labels_chr.data());
      ImPlot::SetupAxisTicks(ImAxis_X1, 1 - 0.5/parts, 0.5/parts,
                             parts, x_labels_chr.data());
      ImPlot::PlotHeatmap("Fitness by layer", fit.data(), max_layers, parts,
                          fit_min, fit_max, nullptr);
      ImPlot::EndPlot();
    }

    ImPlot::PopColormap();
  }
}

void render_layers_age(const std::vector<layers_sequence> &layers_runs)
{
  const float avail(ImGui::GetContentRegionAvail().y * 0.90f);

  for (std::size_t run(layers_runs.size()); run--;)
  {
    const auto &lr(layers_runs[run]);

    if (lr.empty())
      continue;

    id_scope scope(run);

    ImGui::SetNextItemOpen(run + 1 == layers_runs.size(), ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(("Run " + std::to_string(run)).c_str()))
      continue;

    std::vector<unsigned> ys(lr.size());
    std::vector<ultra::individual::age_t> bottom(lr.size()), mean(lr.size()),
                                          top(lr.size());

    for (std::size_t layer(0); layer < lr.size(); ++layer)
    {
      ys[layer] = layer;
      mean[layer] = static_cast<ultra::individual::age_t>(lr.age_mean[layer]);
      bottom[layer] = mean[layer] - lr.age_min[layer];
      top[layer] = lr.age_max[layer] - mean[layer];
    }

    const std::string title("Age by layer - Generation "
                            + std::to_string(lr.generation));
    if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, avail),
                          ImPlotFlags_NoLegend))
    {
      ImPlot::SetupAxes("Age", "Layer",
                        ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

      ImPlot::SetupAxisTicks(ImAxis_Y1, 0, ys.size(), ys.size() + 1);
      ImPlot::GetStyle().ErrorBarWeight = 6;
      ImPlot::GetStyle().ErrorBarSize = 12;
      ImPlot::PlotErrorBars("Age range by layer", mean.data(), ys.data(),
                            bottom.data(), top.data(), ys.size(),
                            ImPlotErrorBarsFlags_Horizontal);
      ImPlot::PlotScatter("Age range by layer", mean.data(), ys.data(),
                          ys.size());
      ImPlot::PlotInfLines("Age limit by layer", lr.age_sup.data(), ys.size());

      for (std::size_t layer(0); layer < lr.age_sup.size(); ++layer)
        if (lr.age_sup[layer])
        {
          const std::string ln("L" + std::to_string(layer));
          const std::string lt("<" + std::to_string(lr.age_sup[layer]));
          ImPlot::TagX(lr.age_sup[layer], ImVec4(1,1,0,0.1),
                       "\n%s\n%s", ln.c_str(), lt.c_str());
        }

      ImPlot::EndPlot();
    }
  }
}

enum class layer_info {age, fitness};
void render_layers(layer_info li)
{
  static std::vector<layers_sequence> layers_runs;

  while (auto data = layers_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (layers_runs.empty() || !layers_runs.back().empty())
        layers_runs.push_back({});
    }
    else
    {
      if (layers_runs.empty())  // just to land on one's feet
        layers_runs.push_back({});

      layers_runs.back().update(*data);
    }
  }

  if (li == layer_info::age)
    render_layers_age(layers_runs);
  else  // layer_info::fitness
    render_layers_fit(layers_runs);
}

void render_monitor(const imgui_app::program &prg, bool *p_open)
{
  const auto fa(prg.free_area());
  ImGui::SetNextWindowPos(ImVec2(fa.x, fa.y));
  ImGui::SetNextWindowSize(ImVec2(fa.w, fa.h));

  static bool show_dynamic_check(true);
  static bool show_population_check(true);
  static bool show_layers_fit_check(true);
  static bool show_layers_age_check(true);

  static bool mxz_dynamic(false);
  static bool mxz_population(false);
  static bool mxz_layers_fit(false);
  static bool mxz_layers_age(false);

  if (ImGui::Begin("Monitor##Window", p_open))
  {
    if (ImGui::CollapsingHeader("GUI Parameters"))
    {
      ImGui::Checkbox("Dynamic", &show_dynamic_check);
      ImGui::SameLine();
      ImGui::Checkbox("Population", &show_population_check);
      ImGui::SameLine();
      ImGui::Checkbox("Layers fit.", &show_layers_fit_check);
      ImGui::SameLine();
      ImGui::Checkbox("Layers age", &show_layers_age_check);
      ImGui::SameLine();
      ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.33f);
      ImGui::SliderInt("##MonitoringWindow", &monitor::window, 0, 8000,
                       "window = %d");
      ImGui::PopItemWidth();
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    const bool show_dynamic(
      !monitor::slog.dynamic_file_path.empty() && show_dynamic_check
      && !(mxz_population && show_population_check)
      && !(mxz_layers_fit && show_layers_fit_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_population(
      !monitor::slog.population_file_path.empty() && show_population_check
      && !(mxz_dynamic && show_dynamic_check)
      && !(mxz_layers_fit && show_layers_fit_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_layers_fit(
      !monitor::slog.layers_file_path.empty() && show_layers_fit_check
      && !(mxz_dynamic && show_dynamic_check)
      && !(mxz_population && show_population_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_layers_age(
      !monitor::slog.layers_file_path.empty() && show_layers_age_check
      && !(mxz_dynamic && show_dynamic_check)
      && !(mxz_population && show_population_check)
      && !(mxz_layers_fit && show_layers_fit_check));

    const int available_width(ImGui::GetContentRegionAvail().x - 4);
    const int available_height(ImGui::GetContentRegionAvail().y - 4);

    const int w1(show_dynamic && show_population ? available_width/2
                                                 : available_width);
    const int h1(show_layers_fit || show_layers_age ? available_height/2
                                                    : available_height);
    if (show_dynamic)
    {
      const auto w(mxz_dynamic ? available_width : w1);
      const auto h(mxz_dynamic ? available_height : h1);

      ImGui::BeginChild("Dynamic##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("DYNAMICS");
      ImGui::SameLine();
      const char *bs(mxz_dynamic ? "Minimise##Dyn" : "Maximise##Dyn");
      if (ImGui::Button(bs))
        mxz_dynamic = !mxz_dynamic;

      render_dynamic();
      ImGui::EndChild();
    }

    if (show_population)
    {
      if (show_dynamic)
        ImGui::SameLine();

      const auto w(mxz_population ? available_width : w1);
      const auto h(mxz_population ? available_height : h1);

      ImGui::BeginChild("Population##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("POPULATION");
      ImGui::SameLine();
      const char *bs(mxz_population ? "Minimise##Pop" : "Maximise##Pop");
      if (ImGui::Button(bs))
        mxz_population = !mxz_population;

      render_population();
      ImGui::EndChild();
    }

    const int w2(show_layers_fit && show_layers_age ? available_width/2
                                                    : available_width);
    const int h2(show_dynamic || show_population ? available_height/2
                                                 : available_height);

    if (show_layers_fit)
    {
      const auto w(mxz_layers_fit ? available_width : w2);
      const auto h(mxz_layers_fit ? available_height : h2);

      ImGui::BeginChild("LayersFitness##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("FITNESS BY LAYER");
      ImGui::SameLine();
      const char *bs(mxz_layers_fit ? "Minimise##LFt" : "Maximise##LFt");
      if (ImGui::Button(bs))
        mxz_layers_fit = !mxz_layers_fit;

      render_layers(layer_info::fitness);
      ImGui::EndChild();
    }

    if (show_layers_age)
    {
      if (show_layers_fit)
        ImGui::SameLine();

      const auto w(mxz_layers_age ? available_width : w2);
      const auto h(mxz_layers_age ? available_height : h2);

      ImGui::BeginChild("LayersAge##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("AGE BY LAYER");
      ImGui::SameLine();
      const char *bs(mxz_layers_age ? "Minimise##LAg" : "Maximise##LAg");
      if (ImGui::Button(bs))
        mxz_layers_age = !mxz_layers_age;

      render_layers(layer_info::age);
      ImGui::EndChild();
    }
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();
}

class read_log_file
{
public:
  explicit read_log_file(const fs::path &);

  std::optional<std::string> get_line();

private:
  std::ifstream file_;
  std::streampos position_ {0};
};

read_log_file::read_log_file(const fs::path &filename)
  : file_(filename)
{
  if (!filename.empty() && !file_)
    throw std::runtime_error("Failed to open file for reading");
}

//
// Reads a single log file into a queue buffer.
// Takes advantage of the fact that a line is complete if a newline has been
// reached.
//
std::optional<std::string> read_log_file::get_line()
{
  if (file_.is_open())
  {
    // Seek to the last known position.
    file_.clear();
    file_.seekg(position_);

    if (std::string line; std::getline(file_, line))
    {
      if (file_.bad())
        throw std::runtime_error("Error occurred while reading the file.");

      if (!file_.eof())
      {
        position_ = file_.tellg();  // update the position for the next read
        return line;
      }
    }
  }

  return {};
}

[[nodiscard]] std::optional<read_log_file> open_log_file(
  const fs::path &filename)
{
  if (filename.empty())
    return {};

  try
  {
    return std::optional<read_log_file>(std::in_place, filename);
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << "Stopped monitoring " << filename << ": " << e.what() << '\n';
    return {};
  }
}

template<typename Queue, typename DataClass>
[[nodiscard]] bool process_log_stream(read_log_file &log_file, Queue &queue,
                                      ultra::timer &last_read,
                                      const std::stop_token &stoken,
                                      const fs::path &filename)
{
  try
  {
    if (auto line(log_file.get_line()); line)
    {
      do
      {
        if (stoken.stop_requested())
          break;

        queue.push(DataClass(*line));
        line = log_file.get_line();
      } while (line);

      last_read.restart();
    }

    return true;
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << "Stopped monitoring " << filename << ": " << e.what() << '\n';
    return false;
  }
}

//
// Asynchronously reads all specified log files into queues for subsequent
// processing.
//
void get_logs(std::stop_token stoken)
{
  try
  {
    auto dynamic_log(open_log_file(monitor::slog.dynamic_file_path));
    auto population_log(open_log_file(monitor::slog.population_file_path));
    auto layers_log(open_log_file(monitor::slog.layers_file_path));

    ultra::timer last_read;

    while (!stoken.stop_requested())
    {
      if (dynamic_log
          && !process_log_stream<decltype(dynamic_queue), dynamic_data>(
                *dynamic_log, dynamic_queue, last_read, stoken,
                monitor::slog.dynamic_file_path))
        dynamic_log.reset();

      if (population_log
          && !process_log_stream<decltype(population_queue), population_line>(
                *population_log, population_queue, last_read, stoken,
                monitor::slog.population_file_path))
        population_log.reset();

      if (layers_log
          && !process_log_stream<decltype(layers_queue), layers_line>(
                *layers_log, layers_queue, last_read, stoken,
                monitor::slog.layers_file_path))
        layers_log.reset();

      if (!dynamic_log && !population_log && !layers_log)
        return;

      const auto elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(
                           last_read.elapsed()));

      std::this_thread::sleep_for(std::clamp(elapsed, 100ms, 3000ms));
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << "Stopped all log monitoring: " << e.what() << '\n';
  }
  catch (...)
  {
    std::cerr << "Stopped all log monitoring: unknown error.\n";
  }
}

// Asynchronously reads all available summary files into queues for subsequent
void monitor::start(const imgui_app::program::settings &settings)
{
  std::jthread t_logs(get_logs);

  imgui_app::program prg(settings);
  prg.run(render_monitor);
}

}  // namespace ultra::wopr
