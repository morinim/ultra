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

#include "dashboard.h"
#include "imgui_app.h"
#include "monitor_data.h"

#include "kernel/gp/primitive/real.h"
#include "utility/timer.h"
#include "utility/ts_queue.h"

#include "implot/implot.h"

#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

using namespace std::chrono_literals;

struct id_scope
{
  explicit id_scope(int id) { ImGui::PushID(id); }
  explicit id_scope(const void *p) { ImGui::PushID(p); }
  explicit id_scope(const char *p) { ImGui::PushID(p); }

  ~id_scope() { ImGui::PopID(); }

  id_scope(const id_scope &) = delete;
  id_scope &operator=(const id_scope &) = delete;
};

void render_dynamic_series(const char *mean_label, const char *max_label,
                           const double *xs, const double *mean,
                           const double *std_dev, const double *max,
                           std::size_t count, bool show_max)
{
  ImPlot::PlotErrorBars(mean_label, xs, mean, std_dev, count,
                        {ImPlotProp_LineColor, ImPlot::GetColormapColor(1),
                         ImPlotProp_Size, 0.0f});
  ImPlot::PlotLine(mean_label, xs, mean, count,
                   {ImPlotProp_Marker, ImPlotMarker_Square});

  if (show_max)
    ImPlot::PlotLine(max_label, xs, max, count,
                     {ImPlotProp_LineColor, ImPlot::GetColormapColor(2)});
}

void render_dynamic(const std::vector<dynamic_sequence> &dynamic_runs,
                    int monitoring_window)
{
  static bool show_best(true);
  static bool show_longest(true);
  static std::vector<int> best_idx;

  if (best_idx.size() < dynamic_runs.size())
    best_idx.resize(dynamic_runs.size(), 0);

  const float avail(ImGui::GetContentRegionAvail().y * 0.85f);

  for (std::size_t run(dynamic_runs.size()); run--;)
  {
    const auto &dr(dynamic_runs[run]);
    if (dr.empty())
      continue;

    id_scope scope(run);

    ImGui::SetNextItemOpen(run + 1 == dynamic_runs.size(), ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(("Run " + std::to_string(run)).c_str()))
      continue;

    if (ImGui::BeginTabBar("DynamicTabBar"))
    {
      const auto &xs(dr.xs);

      std::size_t window(monitoring_window
                         ? static_cast<std::size_t>(monitoring_window)
                         : std::numeric_limits<std::size_t>::max());
      window = std::min<std::size_t>(xs.size(), window);

      const auto get_window([&window](const auto &vect)
      {
        return vect.data() + vect.size() - window;
      });

      if (ImGui::BeginTabItem("Fitness dynamic"))
      {
        if (dr.best_prg.empty())
          best_idx[run] = 0;
        else
        {
          best_idx[run] = std::clamp(
            best_idx[run], 0, static_cast<int>(dr.best_prg.size() - 1));
          ImGui::Combo(
            "Best programs", &best_idx[run],
            [](void *data, int index) -> const char *
            {
              const auto &programs(
                *static_cast<const std::vector<std::string> *>(data));
              return programs[programs.size() - 1 - index].c_str();
            },
            const_cast<std::vector<std::string> *>(&dr.best_prg),
            static_cast<int>(dr.best_prg.size()));
        }

        ImGui::SameLine();
        ImGui::Checkbox("Best##show", &show_best);

        if (ImPlot::BeginPlot("##Fitness by generation", ImVec2(-1, avail),
                              ImPlotFlags_NoTitle))
        {
          ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

          ImPlot::SetupAxes("Generation", "Fit",
                            ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

          render_dynamic_series(
            "Avg & StdDev", "Best", get_window(xs), get_window(dr.fit_mean),
            get_window(dr.fit_std_dev), get_window(dr.fit_best),
            window, show_best);

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

          render_dynamic_series(
            "Len Avg & StdDev", "Longest", get_window(xs),
            get_window(dr.len_mean), get_window(dr.len_std_dev),
            get_window(dr.len_max), window, show_longest);

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

void render_population(
  const std::vector<population_sequence> &population_runs)
{
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

          ImPlot::PlotShaded("Entropy", pr.xs.data(), pr.fit_entropy.data(),
                             pr.xs.size(),
                             -std::numeric_limits<double>::infinity(),
                             {ImPlotProp_FillAlpha, 0.25f});
          ImPlot::PlotLine("Entropy", pr.xs.data(), pr.fit_entropy.data(),
                           pr.xs.size());

          ImPlot::EndPlot();
        }

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
  }
}

void render_layers_fit(const std::vector<layers_line> &layers_runs)
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
    if (!ind_max)
      continue;

    const std::size_t parts(std::min<std::size_t>(ind_max, 100));

    static std::vector<std::string> x_labels, y_labels;
    static std::vector<const char *> x_labels_chr, y_labels_chr;
    static std::vector<double> x_positions, y_positions;
    static std::vector<double> fit;

    if (parts == 1)
    {
      x_labels = {"1"};
      x_positions = {0.5};
    }
    else
    {
      const auto d_parts(static_cast<double>(parts));
      x_labels = {"1", std::to_string(ind_max)};
      x_positions = {0.5 / d_parts, 1.0 - 0.5 / d_parts};
    }

    x_labels_chr.resize(x_labels.size());
    std::ranges::transform(x_labels, x_labels_chr.begin(),
                           [](const auto &str) { return str.data(); });

    const auto d_max_layers(static_cast<double>(max_layers));
    y_labels.resize(max_layers);
    y_positions.resize(max_layers);
    for (std::size_t layer(0); layer < max_layers; ++layer)
    {
      y_labels[layer] = "L" + std::to_string(layer);
      y_positions[layer] = 1.0 - (static_cast<double>(layer) + 0.5)
                                 / d_max_layers;
    }

    y_labels_chr.resize(max_layers);
    std::ranges::transform(y_labels, y_labels_chr.begin(),
                           [](const auto &str) { return str.data(); });

    g.seed(lr.generation);

    const double fit_max(std::ranges::max(lr.fit_max));
    const double fit_min(std::ranges::min(lr.fit_min));

    fit.assign(max_layers * parts, fit_min);

    for (std::size_t layer(0); layer < max_layers; ++layer)
    {
      const auto mean(lr.fit_mean[layer]);
      const auto stddev(lr.fit_std_dev[layer]);
      const auto min(lr.fit_min[layer]);
      const auto max(lr.fit_max[layer]);
      const std::size_t full(parts * lr.individuals[layer] / ind_max);

      if (ultra::issmall(stddev))
      {
        const auto val(std::clamp(mean, min, max));
        for (std::size_t p(0); p < full; ++p)
          fit[layer * parts + p] = val;
      }
      else
      {
        std::normal_distribution dist(mean, stddev);

        for (std::size_t p(0); p < full; ++p)
          fit[layer * parts + p] = std::clamp(dist(g), min, max);
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

      ImPlot::SetupAxisTicks(ImAxis_Y1, y_positions.data(),
                             static_cast<int>(max_layers),
                             y_labels_chr.data());
      ImPlot::SetupAxisTicks(ImAxis_X1, x_positions.data(),
                             static_cast<int>(x_positions.size()),
                             x_labels_chr.data());
      ImPlot::PlotHeatmap("Fitness by layer", fit.data(),
                          static_cast<int>(max_layers),
                          static_cast<int>(parts),
                          fit_min, fit_max, nullptr);
      ImPlot::EndPlot();
    }

    ImPlot::PopColormap();
  }
}

void render_layers_age(const std::vector<layers_line> &layers_runs)
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

    static std::vector<unsigned> ys;
    static std::vector<ultra::individual::age_t> bottom, mean, top;

    ys.resize(lr.size());
    bottom.resize(lr.size());
    mean.resize(lr.size());
    top.resize(lr.size());

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
      ImPlot::PlotErrorBars("Age range by layer", mean.data(), ys.data(),
                            bottom.data(), top.data(), ys.size(),
                            {ImPlotProp_LineWeight, 6.0f,
                             ImPlotProp_Size, 12.0f,
                             ImPlotProp_Flags, ImPlotErrorBarsFlags_Horizontal});
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

void render_monitor(const imgui_app::program &prg, bool *p_open,
                    monitor::options &options, monitor_state &state)
{
  state.update();

  const auto fa(prg.free_area());
  ImGui::SetNextWindowPos(ImVec2(fa.x, fa.y));
  ImGui::SetNextWindowSize(ImVec2(fa.w, fa.h));

  static bool show_dynamic_check(true);
  static bool show_population_check(true);
  static bool show_layers_fit_check(true);
  static bool show_layers_age_check(true);

  static std::optional<std::size_t> maximised;

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
      ImGui::SliderInt("##MonitoringWindow", &options.window, 0, 8000,
                       "window = %d");
      ImGui::PopItemWidth();
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    std::array panels
    {
      dashboard_panel
      {
        "Dynamic##ChildWindow", "DYNAMICS",
        !options.slog.dynamic_file_path.empty() && show_dynamic_check,
        [&options, &state]
        { render_dynamic(state.dynamic_runs, options.window); }
      },
      dashboard_panel
      {
        "Population##ChildWindow", "POPULATION",
        !options.slog.population_file_path.empty() && show_population_check,
        [&state] { render_population(state.population_runs); }
      },
      dashboard_panel
      {
        "LayersFitness##ChildWindow", "FITNESS BY LAYER",
        !options.slog.layers_file_path.empty() && show_layers_fit_check,
        [&state] { render_layers_fit(state.layers_runs); }
      },
      dashboard_panel
      {
        "LayersAge##ChildWindow", "AGE BY LAYER",
        !options.slog.layers_file_path.empty() && show_layers_age_check,
        [&state] { render_layers_age(state.layers_runs); }
      }
    };
    render_dashboard(panels, maximised);
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();
}

class read_log_file
{
public:
  explicit read_log_file(const fs::path &);

  [[nodiscard]] bool is_open() const { return file_.is_open(); }
  std::optional<std::string> get_line();

private:
  std::ifstream file_;
  std::streampos position_ {0};
};

read_log_file::read_log_file(const fs::path &filename)
  : file_(filename)
{
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

    if (std::string line; std::getline(file_, line) && !file_.eof())
    {
      position_ = file_.tellg();  // update the position for the next read
      return line;
    }

    if (file_.bad())
      throw std::runtime_error("Error occurred while reading the file.");
  }

  return {};
}

[[nodiscard]] std::optional<read_log_file> open_log_file(
  const fs::path &filename)
{
  if (filename.empty())
    return {};

  read_log_file file(filename);
  if (!file.is_open())
  {
    std::cerr << "Stopped monitoring " << filename
              << ": Failed to open file for reading\n";
    return {};
  }

  return file;
}

template<typename DataClass, typename Queue>
[[nodiscard]] bool process_log_stream(read_log_file &log_file, Queue &queue,
                                      ultra::timer &last_read,
                                      const std::stop_token &stoken,
                                      const fs::path &filename)
{
  try
  {
    bool read_any(false);

    while (!stoken.stop_requested())
    {
      const auto line(log_file.get_line());
      if (!line)
        break;

      queue.push(DataClass(*line));
      read_any = true;
    }

    if (read_any)
      last_read.restart();

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
void get_logs(std::stop_token stoken, const ultra::search_log &slog,
              monitor_state &state)
{
  try
  {
    auto dynamic_log(open_log_file(slog.dynamic_file_path));
    auto population_log(open_log_file(slog.population_file_path));
    auto layers_log(open_log_file(slog.layers_file_path));

    ultra::timer last_read;

    std::condition_variable_any wake;
    std::mutex wake_mutex;

    while (!stoken.stop_requested())
    {
      if (dynamic_log
          && !process_log_stream<dynamic_data>(
                *dynamic_log, state.dynamic_queue, last_read, stoken,
                slog.dynamic_file_path))
        dynamic_log.reset();

      if (population_log
          && !process_log_stream<population_line>(
                *population_log, state.population_queue, last_read, stoken,
                slog.population_file_path))
        population_log.reset();

      if (layers_log
          && !process_log_stream<layers_line>(
                *layers_log, state.layers_queue, last_read, stoken,
                slog.layers_file_path))
        layers_log.reset();

      if (!dynamic_log && !population_log && !layers_log)
        return;

      const auto elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(
                           last_read.elapsed()));

      const auto delay(std::clamp(elapsed, 100ms, 3000ms));
      std::unique_lock lock(wake_mutex);
      wake.wait_for(lock, stoken, delay, [] { return false; });
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
void monitor::start(options options)
{
  monitor_state state;
  std::jthread t_logs(get_logs, std::cref(options.slog), std::ref(state));

  imgui_app::program::settings settings;
  settings.title = "WOPR";
  settings.flags |= SDL_WINDOW_MAXIMIZED;
  settings.demo = options.imgui_demo;

  imgui_app::program prg(settings);
  prg.run(
    [&options, &state](const auto &program, bool *open)
    {
      render_monitor(program, open, options, state);
    });
}

}  // namespace ultra::wopr
