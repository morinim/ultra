/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "argh/argh.h"
#include "kernel/exceptions.h"
#include "kernel/search_log.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"
#include "utility/timer.h"
#include "utility/ts_queue.h"

#include "imgui_app.h"


ultra::search_log slog {};
std::set<std::filesystem::path> datasets {};
bool imgui_demo_panel {false};


[[nodiscard]] std::string random_string();


/*********************************************************************
 * Dynamic file - related data structures
 ********************************************************************/
struct dynamic_data
{
  explicit dynamic_data(const std::string &);

  bool new_run {false};
  unsigned generation {0};

  ultra::fitnd fit_best {};
  ultra::fitnd fit_mean {};
  ultra::fitnd fit_std_dev {};
  ultra::fitnd fit_min {};

  unsigned len_mean {};
  double len_std_dev {};
  unsigned len_max {};

  std::string best_prg {};
};

dynamic_data::dynamic_data(const std::string &line) : new_run(line.empty())
{
  if (!new_run)
  {
    std::istringstream ss(line);
    if (!(ss >> generation)
        || !(ss >> fit_best)
        || !(ss >> fit_mean)
        || !(ss >> fit_std_dev)
        || !(ss >> fit_min)
        || !(ss >> len_mean)
        || !(ss >> len_std_dev)
        || !(ss >> len_max)
        || !std::getline(ss, best_prg))
      throw ultra::exception::data_format("Cannot parse dynamic file line");
  }
}

ultra::ts_queue<dynamic_data> dynamic_queue;

struct dynamic_sequence
{
  std::vector<double> xs {};
  std::vector<double> fit_best {};
  std::vector<double> fit_mean {};
  std::vector<double> fit_std_dev {};
  std::vector<double> len_mean {};
  std::vector<double> len_std_dev {};
  std::vector<double> len_max {};

  std::vector<std::string> best_prg {};

  [[nodiscard]] bool empty() const noexcept { return xs.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return xs.size(); }

  void push_back(const dynamic_data &dd)
  {
    xs.push_back(xs.size());
    fit_best.push_back(dd.fit_best[0]);
    fit_mean.push_back(dd.fit_mean[0]);
    fit_std_dev.push_back(dd.fit_std_dev[0]);
    len_mean.push_back(dd.len_mean);
    len_std_dev.push_back(dd.len_std_dev);
    len_max.push_back(dd.len_max);

    if (best_prg.empty() || best_prg.back() != dd.best_prg)
      best_prg.push_back(dd.best_prg);
  }
};

std::vector<dynamic_sequence> dynamic_runs;

/*********************************************************************
 * Population-related data structures
 ********************************************************************/
struct population_line
{
  explicit population_line(const std::string &);

  bool new_run {false};
  unsigned generation {0};

  std::vector<double> fit {};
  std::vector<double> obs {};
};

population_line::population_line(const std::string &line)
  : new_run(line.empty())
{
  if (new_run)
    return;

  std::istringstream ss(line);
  if (!(ss >> generation))
  {
    std::cout << line << std::endl;
    throw ultra::exception::data_format(
      "Cannot parse population file line (missing generation)");
  }

  ultra::fitnd fit_val;
  std::size_t obs_val;

  while (ss >> fit_val)
  {
    if (!(ss >> obs_val))
      throw ultra::exception::data_format(
        "Cannot parse population file line (missing observations)");

    for (std::size_t i(0); i < obs_val; ++i)
      fit.push_back(fit_val[0]);
    obs.push_back(obs_val);
  }
}

ultra::ts_queue<population_line> population_queue;

struct population_sequence
{
  std::vector<double> fit {};
  std::vector<double> obs {};

  std::vector<double> fit_entropy {};

  unsigned generation {0};

  [[nodiscard]] bool empty() const noexcept { return fit.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return fit.size(); }

  void push_back(population_line &pl)
  {
    generation = pl.generation;

    fit = std::move(pl.fit);
    obs = std::move(pl.obs);

    fit_entropy.push_back(calculate_entropy());
  }

  // Returns the entropy of the distribution.
  //
  // \f$H(X)=-\sum_{i=1}^n p(x_i) \dot log_b(p(x_i))\f$
  //
  // Offline algorithm: https://en.wikipedia.org/wiki/Online_algorithm.
  [[nodiscard]] double calculate_entropy() const
  {
    const double c(1.0 / std::log(2.0));

    const auto pop_size(std::accumulate(obs.begin(), obs.end(), 0.0));

    double h(0.0);
    for (auto x : obs)
    {
      const auto p(x / pop_size);

      h -= p * std::log(p) * c;
    }

    return h;
  }
};

std::vector<population_sequence> population_runs;

/*********************************************************************
 * Layer-related data structures
 ********************************************************************/
struct layers_line
{
  explicit layers_line(const std::string &);

  bool new_run {false};
  unsigned generation {};

  std::vector<ultra::individual::age_t> age_sup {};
  std::vector<double> age_mean {};
  std::vector<double> age_std_dev {};
  std::vector<ultra::individual::age_t> age_min {};
  std::vector<ultra::individual::age_t> age_max {};

  std::vector<double> fit_mean {};
  std::vector<double> fit_std_dev {};
  std::vector<double> fit_min {};
  std::vector<double> fit_max {};

  std::vector<std::size_t> individuals {};
};

layers_line::layers_line(const std::string &line) : new_run(line.empty())
{
  if (new_run)
    return;

  std::istringstream ss(line);
  if (!(ss >> generation))
  {
    std::cout << line << std::endl;
    throw ultra::exception::data_format(
      "Cannot parse layers file line (missing generation)");
  }

  decltype(age_sup)::value_type age_sup_val;
  decltype(age_mean)::value_type age_mean_val;
  decltype(age_std_dev)::value_type age_std_dev_val;
  decltype(age_min)::value_type age_min_val;
  decltype(age_max)::value_type age_max_val;
  ultra::fitnd fit_mean_val;
  ultra::fitnd fit_std_dev_val;
  ultra::fitnd fit_min_val;
  ultra::fitnd fit_max_val;
  decltype(individuals)::value_type individuals_val;

  while (ss >> age_sup_val)
  {
    if (!(ss >> age_mean_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing age mean)");

    if (!(ss >> age_std_dev_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing age standard deviation)");

    if (!(ss >> age_min_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing age minimum)");

    if (!(ss >> age_max_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing age maximum)");

    if (!(ss >> fit_mean_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing fitness mean)");

    if (!(ss >> fit_std_dev_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing fitness standard deviation)");

    if (!(ss >> fit_min_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing fitness minimum)");

    if (!(ss >> fit_max_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing fitness maximum)");

    if (!(ss >> individuals_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing number of individuals)");

    age_sup.push_back(age_sup_val);
    age_mean.push_back(age_mean_val);
    age_std_dev.push_back(age_std_dev_val);
    age_min.push_back(age_min_val);
    age_max.push_back(age_max_val);

    fit_mean.push_back(fit_mean_val[0]);
    fit_std_dev.push_back(fit_std_dev_val[0]);
    fit_min.push_back(fit_min_val[0]);
    fit_max.push_back(fit_max_val[0]);

    individuals.push_back(individuals_val);
  }
}

ultra::ts_queue<layers_line> layers_queue;

struct layers_sequence
{
  std::vector<ultra::individual::age_t> age_sup {};
  std::vector<double> age_mean {};
  std::vector<double> age_std_dev {};
  std::vector<ultra::individual::age_t> age_min {};
  std::vector<ultra::individual::age_t> age_max {};

  std::vector<double> fit_mean {};
  std::vector<double> fit_std_dev {};
  std::vector<double> fit_min {};
  std::vector<double> fit_max {};

  std::vector<std::size_t> individuals {};

  unsigned generation {0};

  [[nodiscard]] bool empty() const noexcept { return age_sup.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return age_sup.size(); }

  void push_back(layers_line &ld)
  {
    generation = ld.generation;

    age_sup = std::move(ld.age_sup);
    age_mean = std::move(ld.age_mean);
    age_std_dev = std::move(ld.age_std_dev);
    age_min = std::move(ld.age_min);
    age_max = std::move(ld.age_max);

    fit_mean = std::move(ld.fit_mean);
    fit_std_dev = std::move(ld.fit_std_dev);
    fit_min = std::move(ld.fit_min);
    fit_max = std::move(ld.fit_max);

    individuals = std::move(ld.individuals);
  }
};

std::vector<layers_sequence> layers_runs;

/*********************************************************************
 * Rendering
 ********************************************************************/
void render_dynamic()
{
  if (const auto data(dynamic_queue.try_pop()); data)
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (dynamic_runs.empty() || !dynamic_runs.back().empty())
        dynamic_runs.push_back({});
    }
    else
      dynamic_runs.back().push_back(*data);
  }

  static bool show_best(true);
  static bool show_longer(true);

  for (std::size_t run(dynamic_runs.size()); run--;)
  {
    const auto &dr(dynamic_runs[run]);

    if (run == dr.size() - 1)
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    const std::string run_string("Run " + std::to_string(run));
    if (ImGui::CollapsingHeader(run_string.c_str()))
    {
      const auto &xs(dr.xs);

      if (ImGui::BeginTabBar("DynamicTabBar"))
      {
        if (ImGui::BeginTabItem("Fitness dynamic"))
        {
          static int current_best_prg_index(0);
          if (!dr.best_prg.empty())
          {
            std::string best_prg;
            for (std::size_t i(dr.best_prg.size()); i; --i)
              best_prg += dr.best_prg[i - 1] + std::string(1, '\0');

            ImGui::Combo("Best programs", &current_best_prg_index,
                         best_prg.data());
          }
          ImGui::SameLine();
          ImGui::Checkbox("Best", &show_best);

          if (ImPlot::BeginPlot("##Fitness by generation", ImVec2(-1, -1),
                                ImPlotFlags_NoTitle))
          {
            ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

            ImPlot::SetupAxes(
              "Generation", "Fit",
              ImPlotAxisFlags_AutoFit,  // ImPlotAxisFlags_None
              ImPlotAxisFlags_AutoFit);

            ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
            ImPlot::PlotErrorBars("Avg & StdDev",
                                  xs.data(),
                                  dr.fit_mean.data(),
                                  dr.fit_std_dev.data(),
                                  xs.size());
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
            ImPlot::PlotLine("Avg & StdDev",
                             xs.data(),
                             dr.fit_mean.data(),
                             xs.size());

            if (show_best)
            {
              ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
              ImPlot::PlotLine("Best",
                               xs.data(),
                               dr.fit_best.data(),
                               xs.size());
            }

            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Length dynamic"))
        {
          ImGui::Checkbox("Longer", &show_longer);

          if (ImPlot::BeginPlot("##Length by generation", ImVec2(-1, -1),
                                ImPlotFlags_NoTitle))
          {
            ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

            ImPlot::SetupAxes(
              "Generation", "Length",
              ImPlotAxisFlags_AutoFit,  // ImPlotAxisFlags_None
              ImPlotAxisFlags_AutoFit);

            ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
            ImPlot::PlotErrorBars("Len Avg & StdDev",
                                  xs.data(),
                                  dr.len_mean.data(),
                                  dr.len_std_dev.data(),
                                  xs.size());
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
            ImPlot::PlotLine("Len Avg & StdDev",
                             xs.data(),
                             dr.len_mean.data(),
                             xs.size());

            if (show_longer)
            {
              ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
              ImPlot::PlotLine("Longer",
                               xs.data(),
                               dr.len_max.data(),
                               xs.size());
            }

            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    }
  }
}

void render_population()
{
  if (auto data(population_queue.try_pop()); data)
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (population_runs.empty() || !population_runs.back().empty())
        population_runs.push_back({});
    }
    else
      population_runs.back().push_back(*data);
  }

  for (std::size_t run(population_runs.size()); run--;)
  {
    if (run == population_runs.size() - 1)
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    const std::string run_string("Run " + std::to_string(run));
    if (ImGui::CollapsingHeader(run_string.c_str()))
    {
      const auto &pr(population_runs[run]);

      if (ImGui::BeginTabBar("PopulationTabBar"))
      {
        if (ImGui::BeginTabItem("Fitness histogram"))
        {
          const std::string title("Generation "
                                  + std::to_string(pr.generation)
                                  + "##Population");
          if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1),
                                ImPlotFlags_NoLegend))
          {
            ImPlot::SetupAxes("Fitness", "Individuals",
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);

            //ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            //ImPlot::SetNextMarkerStyle(ImPlotMarker_Square, 6,
            //                           ImPlot::GetColormapColor(1), IMPLOT_AUTO,
            //                           ImPlot::GetColormapColor(1));

            //ImPlot::PlotScatter("Distribution",
            //                    pr.fit.data(), pr.obs.data(), pr.size());

            //ImPlot::PopStyleVar();
            ImPlot::PlotHistogram("##Population fitness histogram",
                                  pr.fit.data(), pr.size(),
                                  std::min<std::size_t>(50, pr.size()/10));
            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Fitness entropy"))
        {
          if (ImPlot::BeginPlot("", ImVec2(-1, -1),
                                ImPlotFlags_NoLegend))
          {
            std::vector<double> xs(pr.fit_entropy.size());
            std::iota(xs.begin(), xs.end(), 0.0);

            ImPlot::SetupAxes("Generation", "Entropy",
                              ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded("Entropy", xs.data(),
                               pr.fit_entropy.data(), xs.size(),
                               -std::numeric_limits<double>::infinity());
            ImPlot::PlotLine("Entropy", xs.data(), pr.fit_entropy.data(),
                             xs.size());
            ImPlot::PopStyleVar();

            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
      }
    }
  }
}

void render_layers_fit()
{
  if (auto data(layers_queue.try_pop()); data)
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (layers_runs.empty() || !layers_runs.back().empty())
        layers_runs.push_back({});
    }
    else
      layers_runs.back().push_back(*data);
  }

  static std::minstd_rand g;

  for (std::size_t run(layers_runs.size()); run--;)
  {
    if (run == layers_runs.size() - 1)
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    const std::string run_string("Run " + std::to_string(run));
    if (ImGui::CollapsingHeader(run_string.c_str()))
    {
      const auto &lr(layers_runs[run]);

      if (lr.empty())
        continue;

      static unsigned max_layers(0);
      if (lr.generation == 0)
        max_layers = lr.size();
      else if (max_layers < lr.size())
        max_layers = lr.size();

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
          std::normal_distribution fit_nd(lr.fit_mean[layer],
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

      ImPlot::ColormapScale("Fit Scale", fit_min, fit_max, ImVec2(80, -1));
      ImGui::SameLine();
      if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1),
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
}

void render_layers_age()
{
  if (auto data(layers_queue.try_pop()); data)
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (layers_runs.empty() || !layers_runs.back().empty())
        layers_runs.push_back({});
    }
    else
      layers_runs.back().push_back(*data);
  }

  static std::minstd_rand g;

  for (std::size_t run(layers_runs.size()); run--;)
  {
    if (run == layers_runs.size() - 1)
      ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    const std::string run_string("Run " + std::to_string(run));
    if (ImGui::CollapsingHeader(run_string.c_str()))
    {
      const auto &lr(layers_runs[run]);

      if (lr.empty())
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
      if (ImPlot::BeginPlot(title.c_str(), ImVec2(-1, -1),
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
        ImPlot::PlotInfLines("Age limit by layer", lr.age_sup.data(),
                             ys.size());

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
}

void render(const imgui_app::program &prg, bool *p_open)
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

  if (ImGui::Begin("Main##Window", p_open))
  {
    ImGui::Checkbox("Dynamic", &show_dynamic_check);
    ImGui::SameLine();
    ImGui::Checkbox("Population", &show_population_check);
    ImGui::SameLine();
    ImGui::Checkbox("Layers fit.", &show_layers_fit_check);
    ImGui::SameLine();
    ImGui::Checkbox("Layers age", &show_layers_age_check);
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    const bool show_dynamic(
      !slog.dynamic_file_path.empty() && show_dynamic_check
      && !(mxz_population && show_population_check)
      && !(mxz_layers_fit && show_layers_fit_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_population(
      !slog.population_file_path.empty() && show_population_check
      && !(mxz_dynamic && show_dynamic_check)
      && !(mxz_layers_fit && show_layers_fit_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_layers_fit(
      !slog.layers_file_path.empty() && show_layers_fit_check
      && !(mxz_dynamic && show_dynamic_check)
      && !(mxz_population && show_population_check)
      && !(mxz_layers_age && show_layers_age_check));
    const bool show_layers_age(
      !slog.layers_file_path.empty() && show_layers_age_check
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
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("DYNAMICS");
      ImGui::SameLine();
      const std::string bs(mxz_dynamic ? "Minimize##Dyn" : "Maximize##Dyn");
      if (ImGui::Button(bs.c_str()))
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
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("POPULATION");
      ImGui::SameLine();
      const std::string bs(mxz_population ? "Minimize##Pop" : "Maximize##Pop");
      if (ImGui::Button(bs.c_str()))
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
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("FITNESS BY LAYER");
      ImGui::SameLine();
      const std::string bs(mxz_layers_fit ? "Minimize##LFt" : "Maximize##LFt");
      if (ImGui::Button(bs.c_str()))
        mxz_layers_fit = !mxz_layers_fit;

      render_layers_fit();
      ImGui::EndChild();
    }

    if (show_layers_age)
    {
      if (show_layers_fit)
        ImGui::SameLine();

      const auto w(mxz_layers_age ? available_width : w2);
      const auto h(mxz_layers_age ? available_height : h2);

      ImGui::BeginChild("LayersAge##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("AGE BY LAYER");
      ImGui::SameLine();
      const std::string bs(mxz_layers_fit ? "Minimize##LAg" : "Maximize##LAg");
      if (ImGui::Button(bs.c_str()))
        mxz_layers_age = !mxz_layers_age;

      render_layers_age();
      ImGui::EndChild();
    }
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();
}

/*********************************************************************
 * Misc
 ********************************************************************/
std::string random_string()
{
  constexpr std::size_t length(10);

  static const std::string charset =
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

void read_file(std::stop_token stoken,
               const std::filesystem::path &filename,
               ultra::ts_queue<std::string> &buffer)
{
  std::ifstream file(filename);
  if (!file)
    throw std::runtime_error("Failed to open file for reading");

  std::streampos position(0);

  while (!stoken.stop_requested())
  {
    std::string line;

    // Seek to the last known position.
    file.clear();
    file.seekg(position);

    while (std::getline(file, line))
      if (!file.eof())
      {
        position = file.tellg();  // update the position for the next read
        buffer.push(line);
      }

    if (file.bad())
      throw std::runtime_error("Error occurred while reading the file.");

    assert(file.eof());

    // Small delay before checking for new data.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(150ms);
  }
}

void get_data(std::stop_token stoken)
{
  assert(!slog.dynamic_file_path.empty()
         || !slog.layers_file_path.empty()
         || !slog.population_file_path.empty());

  std::jthread read_dynamic;
  ultra::ts_queue<std::string> dynamic_buffer;
  if (!slog.dynamic_file_path.empty())
    read_dynamic = std::jthread(read_file, slog.dynamic_file_path,
                                std::ref(dynamic_buffer));

  std::jthread read_population;
  ultra::ts_queue<std::string> population_buffer;
  if (!slog.population_file_path.empty())
    read_population = std::jthread(read_file, slog.population_file_path,
                                   std::ref(population_buffer));

  std::jthread read_layers;
  ultra::ts_queue<std::string> layers_buffer;
  if (!slog.layers_file_path.empty())
    read_layers = std::jthread(read_file, slog.layers_file_path,
                               std::ref(layers_buffer));

  ultra::timer last_read;

  while (!stoken.stop_requested())
  {
    if (const auto line(dynamic_buffer.try_pop()); line)
    {
      dynamic_queue.push(dynamic_data(*line));
      last_read.restart();
    }

    if (const auto line(population_buffer.try_pop()); line)
    {
      population_queue.push(population_line(*line));
      last_read.restart();
    }

    if (const auto line(layers_buffer.try_pop()); line)
    {
      layers_queue.push(layers_line(*line));
      last_read.restart();
    }

    using namespace std::chrono_literals;
    const auto elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(
                         last_read.elapsed()));
    std::this_thread::sleep_for(std::min(3000ms, elapsed));
  }
}

/*********************************************************************
 * Command line
 ********************************************************************/
enum class cmdl_result {error, help, monitor, test};

void cmdl_usage()
{
  std::cout
    << R"( _       ___   ___   ___ )" << '\n'
    << R"(\ \    // / \ | |_) | |_))" << '\n'
    << R"( \_\/\/ \_\_/ |_|   |_| \)"
    << "\n\n"
    << "GREETINGS PROFESSOR FALKEN.\n"
    << "\n"
    << "Please enter your selection:\n"
    << "\n"
    <<
  "> wopr monitor [data_folder]\n"
  "    The data folder must contain at least a search log produced by Ultra.\n"
  "    If missing, the current working directory is used.\n"
  "\n"
  "> wopr test    [test_folder]\n"
  "    The test folder must contain at least a dataset and optionally a test\n"
  "    configuration file. If missing, the current working directory is used.\n"
  "\n"
  "Available switches:\n"
  "\n"
  "--dynamic      filename\n"
  "--layers       filename\n"
  "--population   filename\n"
  "    Use a filename different from the default one.\n"
  "\n"
  "-help\n"
  "    Show this help screen.\n"
  "-imguidemo\n"
  "    Enable ImGUI demo panel.\n"
  "\n"
    << "SHALL WE PLAY A GAME?\n\n";
}

std::filesystem::path build_path(const std::filesystem::path &base_dir,
                                 const std::filesystem::path &f,
                                 const std::string &default_filename)
{
  if (f.is_absolute())
  {
    if (std::filesystem::exists(f))
      return f;

    std::cerr << "File " << f << " doesn't exist\n";
  }
  else if (!f.empty())
  {
    if (const auto bf(base_dir / f); std::filesystem::exists(bf))
      return bf;
    else
      std::cerr << "File " << bf << " doesn't exist\n";
  }
  else
  {
    if (const auto bf(base_dir / default_filename);
        std::filesystem::exists(bf))
      return bf;
  }

  return std::filesystem::path{};
}

cmdl_result parse_args(int argc, char *argv[])
{
  argh::parser cmdl;
  cmdl.parse(argc, argv);

  const std::string cmd_monitor("monitor");
  const std::string cmd_test("test");
  const std::set<std::string> cmds({cmd_monitor, cmd_test});

  const auto &pos_args(cmdl.pos_args());

  if (pos_args.size() <= 1 || cmdl[{"-h", "--help"}])
    return cmdl_result::help;

  std::string cmd;
  std::ranges::transform(pos_args[1], std::back_inserter(cmd),
                         [](auto c){ return std::tolower(c); });

  if (!cmds.contains(cmd))
  {
    std::cerr << "Unknown command.\n";
    return cmdl_result::error;
  }

  imgui_demo_panel = cmdl["imguidemo"];

  if (cmd == cmd_monitor)
  {
    const std::filesystem::path data_folder(pos_args.size() <= 2
                                            ? "./" : pos_args[2]);

    if (!std::filesystem::is_directory(data_folder))
    {
      std::cerr << data_folder << " isn't a directory\n";
      return cmdl_result::error;
    }

    slog.base_dir = data_folder;

    using namespace ultra;
    slog.dynamic_file_path = build_path(slog.base_dir,
                                        cmdl("dynamic", "").str(),
                                        search_log::default_dynamic_file);
    slog.layers_file_path = build_path(slog.base_dir,
                                       cmdl("layers", "").str(),
                                       search_log::default_layers_file);
    slog.population_file_path = build_path(slog.base_dir,
                                           cmdl("population", "").str(),
                                           search_log::default_population_file);

    if (slog.dynamic_file_path.empty()
        && slog.layers_file_path.empty()
        && slog.population_file_path.empty())
    {
      std::cerr << "No data file available.\n";
      return cmdl_result::error;
    }

    std::cout << "Dynamic file path: " << slog.dynamic_file_path
              << "\nLayers file path: " << slog.layers_file_path
              << "\nPopulation file path: " << slog.population_file_path
              << '\n';

    return cmdl_result::monitor;
  }
  else if (cmd == cmd_test)
  {
    const std::filesystem::path test_folder(pos_args.size() <= 2
                                            ? "./" : pos_args[2]);

    if (!std::filesystem::is_directory(test_folder))
    {
      std::cerr << test_folder << " isn't a directory.\n";
      return cmdl_result::error;
    }

    for (const auto &entry : std::filesystem::directory_iterator(test_folder))
      if (ultra::iequals(entry.path().extension(), ".csv"))
        datasets.insert(entry.path());

    if (datasets.empty())
    {
      std::cerr << "No dataset available.\n";
      return cmdl_result::error;
    }

    std::cout << "Datasets:";
    for (const auto &d : datasets)
      std::cout << ' ' << d;
    std::cout << '\n';

    return cmdl_result::test;
  }

  return cmdl_result::error;
}

/*********************************************************************
 * MAIN
 ********************************************************************/
void monitor(const imgui_app::program::settings &settings)
{
  std::jthread t_data(get_data);

  imgui_app::program prg(settings);
  prg.run(render);
}

void test(const imgui_app::program::settings &)
{
  std::stop_source source;

  std::vector<std::jthread> threads;

  const auto test_dataset(
    [stop_token = source.get_token()](std::filesystem::path dataset)
    {
      ultra::src::problem prob(dataset);

      prob.insert<ultra::real::sin>();
      prob.insert<ultra::real::cos>();
      prob.insert<ultra::real::add>();
      prob.insert<ultra::real::sub>();
      prob.insert<ultra::real::div>();
      prob.insert<ultra::real::mul>();

      ultra::src::search s(prob);
      return s.run(-1);
    });

  std::vector<std::future<ultra::search_stats<ultra::gp::individual,
                                              double>>> tasks;
  for (const auto &dataset : datasets)
    tasks.push_back(std::async(std::launch::async, test_dataset, dataset));
}

int main(int argc, char *argv[])
{
  const auto result(parse_args(argc, argv));

  if (result == cmdl_result::error)
  {
    std::cerr << "Use `--help` switch for command line description.\n\n"
              << "People sometimes make mistakes.\n";
    return EXIT_FAILURE;
  }

  if (result == cmdl_result::help)
  {
    cmdl_usage();
    return EXIT_SUCCESS;
  }

  imgui_app::program::settings settings;
  settings.w_related.title = "WOPR";
  settings.demo = imgui_demo_panel;

  if (result == cmdl_result::monitor)
    monitor(settings);
  else if (result == cmdl_result::test)
    test(settings);

  return EXIT_SUCCESS;
}
