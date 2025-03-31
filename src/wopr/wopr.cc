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

#include "imgui_app.h"

#include "kernel/exceptions.h"
#include "kernel/search_log.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"
#include "utility/timer.h"
#include "utility/ts_queue.h"

#include "argh/argh.h"

#include <iostream>
#include <map>
#include <cassert>

using namespace std::chrono_literals;


// Monitoring related variables.
ultra::search_log slog {};
int monitoring_window {0};
std::chrono::duration<double> refresh_rate {2s};

// Testing related variables.
bool nogui {false};

struct test_settings
{
  unsigned generations;
  unsigned runs;
  ultra::model_measurements<double> threshold;
};

std::map<std::filesystem::path, test_settings> test_collection;

// Other variables.
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

  void update(population_line &pl)
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

  void update(layers_line &ld)
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


/*********************************************************************
 * Summary-related data structures
 ********************************************************************/
struct summary_data
{
  summary_data() = default;
  explicit summary_data(const tinyxml2::XMLDocument &);
  explicit summary_data(const std::filesystem::path &);

  unsigned runs {0};
  std::chrono::milliseconds elapsed_time {0};
  double success_rate {0.0};

  ultra::fitnd fit_mean {};
  ultra::fitnd fit_std_dev {};

  ultra::fitnd best_fit {-std::numeric_limits<double>::infinity()};
  double best_accuracy {-std::numeric_limits<double>::infinity()};

  unsigned     best_run {};
  std::string  best_prg {};

  std::set<unsigned> good_runs {};
};

summary_data::summary_data(const std::filesystem::path &path)
{
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)
    throw std::invalid_argument("Cannot parse summary file "
                                + path.generic_string());

  *this = summary_data(doc);
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

  const auto h_solutions(handle.FirstChildElement("ultra")
                         .FirstChildElement("solutions"));

  for(const auto *e(h_solutions.FirstChildElement().ToElement()); e;
      e = e->NextSibling()->ToElement())
    if (unsigned run; e->QueryUnsignedText(&run) == tinyxml2::XML_SUCCESS)
      good_runs.insert(run);
}

std::vector<summary_data> summaries;
std::shared_mutex summaries_mutex;

// Doesn't require a mutex since it's compiled before starting testing and
// then used in read-only mode.
std::vector<summary_data> ref_summaries;


/*********************************************************************
 * Rendering
 ********************************************************************/
// Generates a unique identifier for the string `title` within the current
// ImGui scope. `ctx`, if provided, further contributes to uniqueness (e.g.
// for the rendering/monitoring functions it is the run number).
[[nodiscard]] const char *gui_uid(std::string title, unsigned ctx = 0)
{
  static std::string buffer;

  buffer = title + "##" + std::to_string(ctx)
           + std::to_string(ImGui::GetID(title.c_str()));
  return buffer.c_str();
}

void render_runs()
{
  static bool reference_values {true};

  std::vector<unsigned> data;
  {
    std::shared_lock guard(summaries_mutex);
    for (const auto &s : summaries)
      data.push_back(s.runs);
  }

  std::vector<std::string> labels;

  assert(test_collection.size() == ref_summaries.size());
  for (std::size_t i(0); const auto &test : test_collection)
  {
    labels.push_back(test.first.stem().string());
    data.push_back(ref_summaries[i].runs);
    ++i;
  }

  std::vector<const char *> labels_chr(labels.size());
  std::ranges::transform(labels, labels_chr.begin(),
                         [](const auto &str) { return str.data(); });

  const std::vector ilabels = {"Current", "Reference"};
  std::vector<double> positions(test_collection.size());
  std::iota(positions.begin(), positions.end(), 0.0);

  ImGui::Checkbox("Reference values##Test##Runs", &reference_values);

  if (ImPlot::BeginPlot("##Runs##Test", ImVec2(-1, -1), ImPlotFlags_NoTitle))
  {
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes("Dataset", "Runs",
                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), test_collection.size(),
                           labels_chr.data());
    ImPlot::PlotBarGroups(ilabels.data(), data.data(),
                          reference_values ? 2 : 1,
                          test_collection.size(), 0.5, 0, 0);
    ImPlot::EndPlot();
  }
}

void render_success_rate()
{
  static bool reference_values {true};

  std::vector<double> data;
  std::vector<std::string> rlabels;
  {
    std::shared_lock guard(summaries_mutex);
    for (const auto &s : summaries)
    {
      data.push_back(s.success_rate * 100.0);
      rlabels.push_back(std::to_string(s.runs));
    }
  }

  std::vector<std::string> glabels;

  assert(test_collection.size() == ref_summaries.size());
  for (std::size_t i(0); const auto &test : test_collection)
  {
    glabels.push_back(test.first.stem().string());
    data.push_back(ref_summaries[i].success_rate * 100.0);
    ++i;
  }

  std::vector<const char *> glabels_chr(glabels.size());
  std::ranges::transform(glabels, glabels_chr.begin(),
                         [](const auto &str) { return str.data(); });

  const std::vector ilabels = {"Current", "Reference"};
  std::vector<double> positions(test_collection.size());
  std::iota(positions.begin(), positions.end(), 0.0);

  ImGui::Checkbox("Reference values##Test##Success rate", &reference_values);

  if (ImPlot::BeginPlot("##Success rate##Test", ImVec2(-1, -1),
                        ImPlotFlags_NoTitle))
  {
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes("Dataset", "Success rate",
                      ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
    ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), test_collection.size(),
                           glabels_chr.data());
    ImPlot::PlotBarGroups(ilabels.data(), data.data(),
                          reference_values ? 2 : 1,
                          test_collection.size(), 0.5, 0, 0);
    ImPlot::EndPlot();
  }
}

void render_dynamic(bool update)
{
  static std::vector<dynamic_sequence> dynamic_runs;

  if (const auto data = dynamic_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (dynamic_runs.empty() || !dynamic_runs.back().empty())
        dynamic_runs.push_back({});
    }
    else
    {
      static dynamic_sequence buffer;
      buffer.push_back(*data);

      if (update)
        dynamic_runs.back() = buffer;
    }
  }

  static bool show_best(true);
  static bool show_longest(true);

  for (std::size_t run(dynamic_runs.size()); run--;)
  {
    const auto &dr(dynamic_runs[run]);

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader(gui_uid("Run " + std::to_string(run))))
    {
      if (ImGui::BeginTabBar(gui_uid("DynamicTabBar", run)))
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

        if (ImGui::BeginTabItem(gui_uid("Fitness dynamic", run)))
        {
          static int current_best_prg_index(0);
          if (!dr.best_prg.empty())
          {
            std::string best_prg;
            for (std::size_t i(dr.best_prg.size()); i; --i)
              best_prg += dr.best_prg[i - 1] + std::string(1, '\0');

            ImGui::Combo(gui_uid("Best programs", run),
                         &current_best_prg_index, best_prg.data());
          }
          ImGui::SameLine();
          ImGui::Checkbox("Best", &show_best);

          if (ImPlot::BeginPlot(gui_uid("##Fitness by generation", run),
                                ImVec2(-1, -1), ImPlotFlags_NoTitle))
          {
            ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

            ImPlot::SetupAxes("Generation", "Fit",
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);

            ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
            const auto avg_stddev(gui_uid("Avg & StdDev", run));
            ImPlot::PlotErrorBars(avg_stddev,
                                  get_window(xs),
                                  get_window(dr.fit_mean),
                                  get_window(dr.fit_std_dev),
                                  window);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
            ImPlot::PlotLine(avg_stddev,
                             get_window(xs),
                             get_window(dr.fit_mean),
                             window);

            if (show_best)
            {
              ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
              ImPlot::PlotLine("Best",
                               get_window(xs),
                               get_window(dr.fit_best),
                               window);
            }

            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(gui_uid("Length dynamic", run)))
        {
          ImGui::Checkbox(gui_uid("Longest", run), &show_longest);

          if (ImPlot::BeginPlot(gui_uid("##Length by generation", run),
                                ImVec2(-1, -1), ImPlotFlags_NoTitle))
          {
            ImPlot::SetupLegend(ImPlotLocation_South | ImPlotLocation_West);

            ImPlot::SetupAxes(
              "Generation", "Length",
              ImPlotAxisFlags_AutoFit,  // ImPlotAxisFlags_None
              ImPlotAxisFlags_AutoFit);

            const auto avg_stddev(gui_uid("Len Avg & StdDev", run));
            ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
            ImPlot::PlotErrorBars(avg_stddev,
                                  get_window(xs),
                                  get_window(dr.len_mean),
                                  get_window(dr.len_std_dev),
                                  window);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
            ImPlot::PlotLine(avg_stddev,
                             get_window(xs),
                             get_window(dr.len_mean),
                             window);

            if (show_longest)
            {
              ImPlot::SetNextLineStyle(ImPlot::GetColormapColor(2));
              ImPlot::PlotLine(gui_uid("Longest", run),
                               get_window(xs),
                               get_window(dr.len_max),
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
}

void render_population(bool update)
{
  static std::vector<population_sequence> population_runs;

  if (auto data = population_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (population_runs.empty() || !population_runs.back().empty())
        population_runs.push_back({});
    }
    else
    {
      static population_sequence buffer;
      buffer.update(*data);

      if (update)
        population_runs.back() = buffer;
    }
  }

  for (std::size_t run(population_runs.size()); run--;)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    if (ImGui::CollapsingHeader(gui_uid("Run " + std::to_string(run), run)))
    {
      const auto &pr(population_runs[run]);

      if (ImGui::BeginTabBar(gui_uid("PopulationTabBar", run)))
      {
        if (ImGui::BeginTabItem(gui_uid("Fitness histogram", run)))
        {
          const std::string title("Generation "
                                  + std::to_string(pr.generation)
                                  + "##Population");

          if (ImPlot::BeginPlot(gui_uid(title, run), ImVec2(-1, -1),
                                ImPlotFlags_NoLegend))
          {
            ImPlot::SetupAxes("Fitness", "Individuals",
                              ImPlotAxisFlags_AutoFit,
                              ImPlotAxisFlags_AutoFit);
            ImPlot::PlotHistogram(gui_uid("##PopulationFitnessHistogram", run),
                                  pr.fit.data(), pr.fit.size(),
                                  std::min<std::size_t>(50, pr.fit.size()/10));
            ImPlot::EndPlot();
          }

          ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(gui_uid("Fitness entropy", run)))
        {
          const std::string title("Generation "
                                  + std::to_string(pr.generation)
                                  + "##Entropy");
          if (ImPlot::BeginPlot(gui_uid(title, run), ImVec2(-1, -1),
                                ImPlotFlags_NoLegend))
          {
            std::vector<double> xs(pr.fit_entropy.size());
            std::iota(xs.begin(), xs.end(), 0.0);

            ImPlot::SetupAxes("Generation", "Entropy",
                              ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
            ImPlot::PlotShaded(gui_uid("Entropy", run), xs.data(),
                               pr.fit_entropy.data(), xs.size(),
                               -std::numeric_limits<double>::infinity());
            ImPlot::PlotLine(gui_uid("Entropy", run), xs.data(),
                             pr.fit_entropy.data(), xs.size());
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

void render_layers_fit(const std::vector<layers_sequence> &layers_runs)
{
  static std::minstd_rand g;

  for (std::size_t run(layers_runs.size()); run--;)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    if (ImGui::CollapsingHeader(gui_uid("Run " + std::to_string(run), run)))
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

      ImPlot::ColormapScale(gui_uid("Fit Scale", run), fit_min, fit_max,
                            ImVec2(80, -1));
      ImGui::SameLine();
      if (ImPlot::BeginPlot(gui_uid(title, run), ImVec2(-1, -1),
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
        ImPlot::PlotHeatmap(gui_uid("Fitness by layer", run), fit.data(),
                            max_layers, parts, fit_min, fit_max, nullptr);
        ImPlot::EndPlot();
      }

      ImPlot::PopColormap();
    }
  }
}

void render_layers_age(const std::vector<layers_sequence> &layers_runs)
{
  static std::minstd_rand g;

  for (std::size_t run(layers_runs.size()); run--;)
  {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

    if (ImGui::CollapsingHeader(gui_uid("Run " + std::to_string(run), run)))
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
      if (ImPlot::BeginPlot(gui_uid(title, run), ImVec2(-1, -1),
                            ImPlotFlags_NoLegend))
      {
        ImPlot::SetupAxes("Age", "Layer",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

        ImPlot::SetupAxisTicks(ImAxis_Y1, 0, ys.size(), ys.size() + 1);
        ImPlot::GetStyle().ErrorBarWeight = 6;
        ImPlot::GetStyle().ErrorBarSize = 12;
        ImPlot::PlotErrorBars(gui_uid("Age range by layer", run), mean.data(),
                              ys.data(), bottom.data(), top.data(), ys.size(),
                              ImPlotErrorBarsFlags_Horizontal);
        ImPlot::PlotScatter(gui_uid("Age range by layer", run), mean.data(),
                            ys.data(), ys.size());
        ImPlot::PlotInfLines(gui_uid("Age limit by layer", run),
                             lr.age_sup.data(), ys.size());

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

enum class layer_info {age, fitness};
void render_layers(layer_info li, bool update)
{
  static std::vector<layers_sequence> layers_runs;

  if (auto data = layers_queue.try_pop())
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (layers_runs.empty() || !layers_runs.back().empty())
        layers_runs.push_back({});
    }
    else
    {
      static layers_sequence buffer;
      buffer.update(*data);

      if (update)
        layers_runs.back() = buffer;
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

  static ultra::timer last_update;
  const bool update(last_update.elapsed() > refresh_rate);

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
      ImGui::SliderInt("##MonitoringWindow", &monitoring_window, 0, 8000,
                       "window = %d");
    }
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

      render_dynamic(update);
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

      render_population(update);
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

      render_layers(layer_info::fitness, update);
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

      render_layers(layer_info::age, update);
      ImGui::EndChild();
    }
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();

  if (update)
    last_update.restart();
}

void render_test(const imgui_app::program &prg, bool *p_open)
{
  const auto fa(prg.free_area());
  ImGui::SetNextWindowPos(ImVec2(fa.x, fa.y));
  ImGui::SetNextWindowSize(ImVec2(fa.w, fa.h));

  static bool show_best_check(true);
  static bool show_success_rate_check(true);
  static bool show_3_check(true);
  static bool show_4_check(true);

  static bool mxz_best(false);
  static bool mxz_success_rate(false);
  static bool mxz_3(false);
  static bool mxz_4(false);

  if (ImGui::Begin("Test##Window", p_open))
  {
    ImGui::Checkbox("best", &show_best_check);
    ImGui::SameLine();
    ImGui::Checkbox("success rate", &show_success_rate_check);
    ImGui::SameLine();
    ImGui::Checkbox("3", &show_3_check);
    ImGui::SameLine();
    ImGui::Checkbox("4", &show_4_check);
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    const bool show_best(
      show_best_check
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_3 && show_3_check)
      && !(mxz_4 && show_4_check));
    const bool show_success_rate(
      show_success_rate_check
      && !(mxz_best && show_best_check)
      && !(mxz_3 && show_3_check)
      && !(mxz_4 && show_4_check));
    const bool show_3(
      show_3_check
      && !(mxz_best && show_best_check)
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_4 && show_4_check));
    const bool show_4(
      show_4_check
      && !(mxz_best && show_best_check)
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_3 && show_3_check));

    const int available_width(ImGui::GetContentRegionAvail().x - 4);
    const int available_height(ImGui::GetContentRegionAvail().y - 4);

    const int w1(show_best && show_success_rate ? available_width/2
                                    : available_width);
    const int h1(show_3 || show_4 ? available_height/2
                                  : available_height);
    if (show_best)
    {
      const auto w(mxz_best ? available_width : w1);
      const auto h(mxz_best ? available_height : h1);

      ImGui::BeginChild("Best##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("BEST");
      ImGui::SameLine();
      if (const std::string bs(mxz_best ? "Minimize##Best" : "Maximize##Best");
          ImGui::Button(bs.c_str()))
        mxz_best = !mxz_best;

      render_runs();
      ImGui::EndChild();
    }

    if (show_success_rate)
    {
      if (show_best)
        ImGui::SameLine();

      const auto w(mxz_success_rate ? available_width : w1);
      const auto h(mxz_success_rate ? available_height : h1);

      ImGui::BeginChild("Success rate##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Border);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("SUCCESS RATE");
      ImGui::SameLine();
      if (const std::string bs(mxz_success_rate ? "Minimize##SR"
                                                : "Maximize##SR");
          ImGui::Button(bs.c_str()))
        mxz_success_rate = !mxz_success_rate;

      render_success_rate();
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

//
// Reads a single log file into a queue buffer.
// Takes advantage of the fact that a line is complete if a newline has been
// reached.
//
void read_log_file(std::stop_token stoken,
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
    std::this_thread::sleep_for(150ms);
  }
}

//
// Asynchronously reads all specified log files into queues for subsequent
// processing.
//
void get_logs(std::stop_token stoken)
{
  assert(!slog.dynamic_file_path.empty()
         || !slog.layers_file_path.empty()
         || !slog.population_file_path.empty());

  std::jthread read_dynamic;
  ultra::ts_queue<std::string> dynamic_buffer;
  if (!slog.dynamic_file_path.empty())
    read_dynamic = std::jthread(read_log_file, slog.dynamic_file_path,
                                std::ref(dynamic_buffer));

  std::jthread read_population;
  ultra::ts_queue<std::string> population_buffer;
  if (!slog.population_file_path.empty())
    read_population = std::jthread(read_log_file, slog.population_file_path,
                                   std::ref(population_buffer));

  std::jthread read_layers;
  ultra::ts_queue<std::string> layers_buffer;
  if (!slog.layers_file_path.empty())
    read_layers = std::jthread(read_log_file, slog.layers_file_path,
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

    const auto elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(
                         last_read.elapsed()));
    std::this_thread::sleep_for(std::min(3000ms, elapsed));
  }
}

//
// Asynchronously reads all available summary files into queues for subsequent
// processing.
//
void get_summaries(std::stop_token stoken)
{
  assert(!test_collection.empty());

  summaries.resize(test_collection.size());

  while (!stoken.stop_requested())
  {
    for (std::size_t i(0); const auto &test : test_collection)
    {
      ++i;

      const auto &dataset(test.first);
      const std::filesystem::path base_dir(dataset.parent_path());
      const auto xml_fn(base_dir / ultra::summary_from_basename(dataset));
      tinyxml2::XMLDocument summary;
      if (const auto result(summary.LoadFile(xml_fn.c_str()));
          result != tinyxml2::XML_SUCCESS)
        continue;

      if (summary.FirstChild())
      {
        tinyxml2::XMLPrinter printer;
        summary.Print(&printer);
        if (const char *cstr(printer.CStr());
            !cstr || !ultra::crc32::verify_xml_signature(cstr))
          continue;
      }

      std::lock_guard guard(summaries_mutex);
      summaries[i-1] = summary_data(summary);

    }

    std::this_thread::sleep_for(3000ms);
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
  "> wopr monitor [log folder]\n"
  "\n"
  "  The log folder must contain at least one search log produced by Ultra.\n"
  "  If omitted, the current working directory is used.\n"
  "\n"
  "  Available switches:\n"
  "\n"
  "  --basename <name>\n"
  "      Restrict monitoring to log files matching the `basename_*.txt`\n"
  "      format.\n"
  "  --dynamic    <filepath>\n"
  "  --layers     <filepath>\n"
  "  --nogui\n"
  "      Disable the graphical user interface performing the test in headless\n"
  "      mode.\n"
  "  --population <filepath>\n"
  "      Allow monitoring of files with names different from the default\n"
  "      ones.\n"
  "  --refresh <seconds>\n"
  "      Set the refresh rate for updating plots.\n"
  "  --window <nr>\n"
  "      Restrict monitoring window to the last `nr` generations.\n"
  "\n"
  "> wopr test [folder or file]\n"
  "\n"
  "  The argument of the test command must point a to folder containing, at\n"
  "  least, a .csv dataset (and, optionally, a test configuration file) or a\n"
  "  specific file. If no folder is specified, the current working directory\n"
  "  is used.\n"
  "\n"
  "  Available switches:\n"
  "\n"
  "  --generations <nr>\n"
  "      Set the maximum number of generations in a run.\n"
  "  --reference directory <directory>\n"
  "      Specify a directory containing reference results.\n"
  "  --runs <nr>\n"
  "      Perform the specified number of evolutionary runs.\n"
  "  --threshold <val>\n"
  "      Set the success threshold for a run. If the value ends with '%', it\n"
  "      is interpreted as an accuracy measure; otherwise, it is treated as\n"
  "      a fitness value.\n"
  "\n"
  "--help\n"
  "    Show this help screen.\n"
  "--imguidemo\n"
  "    Enable ImGUI demo panel.\n"
  "\n"
    << "SHALL WE PLAY A GAME?\n\n";
}

std::filesystem::path build_path(std::filesystem::path base_dir,
                                 std::filesystem::path f,
                                 const std::string &default_filename = {})
{
  f = f.lexically_normal();

  if (f.is_absolute())
    return f;

  if (base_dir.empty())
    base_dir = "./";

  if (!f.empty())
    return base_dir / f;

  if (!default_filename.empty())
    return base_dir / default_filename;

  return {};
}

[[nodiscard]] bool setup_monitor_cmd(argh::parser &cmdl)
{
  using namespace ultra;

  const auto &pos_args(cmdl.pos_args());
  const std::filesystem::path log_folder(pos_args.size() <= 2
                                         ? "./" : pos_args[2]);

  if (!std::filesystem::is_directory(log_folder))
  {
    std::cerr << log_folder << " isn't a directory\n";
    return false;
  }

  const std::string basename(cmdl("basename", "").str());

  slog.base_dir = log_folder;
  slog.summary_file_path = "";

  slog.dynamic_file_path = build_path(log_folder, cmdl("dynamic", "").str());
  slog.layers_file_path = build_path(log_folder, cmdl("layers", "").str());
  slog.population_file_path = build_path(log_folder,
                                         cmdl("population", "").str());

  std::vector<std::filesystem::path> dynamic_file_paths;
  std::vector<std::filesystem::path> layers_file_paths;
  std::vector<std::filesystem::path> population_file_paths;

  if (slog.dynamic_file_path.empty() || slog.layers_file_path.empty() ||
      slog.population_file_path.empty())
    for (const auto &entry : std::filesystem::directory_iterator(log_folder))
      if (entry.is_regular_file()
          && ultra::iequals(entry.path().extension(), ".txt"))
      {
        const std::string fn(entry.path().filename().string());

        if (const auto def(search_log::default_dynamic_file);
            slog.dynamic_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          dynamic_file_paths.push_back(entry.path());
        }

        if (const auto def(search_log::default_layers_file);
            slog.layers_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          layers_file_paths.push_back(entry.path());
        }

        if (const auto def(search_log::default_population_file);
            slog.population_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          population_file_paths.push_back(entry.path());
        }

        if (dynamic_file_paths.size() > 1
            || layers_file_paths.size() > 1
            || population_file_paths.size() > 1)
        {
          std::cerr << "Too many log files.\n"
                    << "Use `--basename` switch to specify a test.\n";
          return false;
        }
      }

  if (slog.dynamic_file_path.empty())
    slog.dynamic_file_path = dynamic_file_paths.front();
  if (slog.layers_file_path.empty())
    slog.layers_file_path = layers_file_paths.front();
  if (slog.population_file_path.empty())
    slog.population_file_path = population_file_paths.front();

  if (!std::filesystem::exists(slog.dynamic_file_path)
      && !std::filesystem::exists(slog.layers_file_path)
      && !std::filesystem::exists(slog.population_file_path))
  {
    std::cerr << "No log file available.\n";
    return false;
  }

  std::cout << "Dynamic file path: " << slog.dynamic_file_path
            << "\nLayers file path: " << slog.layers_file_path
            << "\nPopulation file path: " << slog.population_file_path
            << '\n';


  if (const auto v(cmdl("window").str()); !v.empty())
  {
    try
    {
      monitoring_window = std::stoi(v);
      std::cout << "Monitoring window: " << monitoring_window << '\n';
    }
    catch (...)
    {
      std::cerr << "Wrong value for monitoring window.\n";
      return false;
    }
  }

  if (const auto v(cmdl("refresh").str()); !v.empty())
  {
    try
    {
      refresh_rate = std::chrono::duration<double>(std::stof(v));
      std::cout << "Refresh rate: " << refresh_rate.count() << "s\n";
    }
    catch (...)
    {
      std::cerr << "Wrong value for refresh rate.\n";
      return false;
    }
  }

  return true;
}

[[nodiscard]] bool setup_test_cmd(argh::parser &cmdl)
{
  using namespace ultra;

  const auto &pos_args(cmdl.pos_args());

  if (const std::filesystem::path test_input(pos_args.size() <= 2
                                             ? "./" : pos_args[2]);
      std::filesystem::is_directory(test_input))
  {
    const std::filesystem::path test_folder(test_input);

    for (const auto &entry : std::filesystem::directory_iterator(test_folder))
      if (ultra::iequals(entry.path().extension(), ".csv"))
        test_collection.insert({entry.path(), {}});
  }
  else if (std::filesystem::exists(test_input))
    test_collection.insert({test_input, {}});
  else
  {
    std::cerr << test_input << " isn't a valid input.\n";
    return false;
  }

  if (test_collection.empty())
  {
    std::cerr << "No dataset available.\n";
    return false;
  }

  std::cout << "Datasets:";
  for (const auto &test : test_collection)
    std::cout << ' ' << test.first;
  std::cout << '\n';

  if (const std::filesystem::path ref_folder(cmdl("reference", "").str());
      ref_folder.empty())
  {
    ref_summaries.resize(test_collection.size(), {});
  }
  else if (std::filesystem::is_directory(ref_folder))
  {
    for (const auto &test : test_collection)
    {
      const auto ref_path(ref_folder
                          / ultra::summary_from_basename(test.first));

      if (std::filesystem::exists(ref_path))
        ref_summaries.push_back(summary_data(ref_path));
      else
        ref_summaries.push_back({});
    }
  }
  else
  {
    std::cerr << ref_folder << " isn't a directory.\n";
    return false;
  }

  unsigned generations(100);
  if (const auto v(cmdl("generations").str()); !v.empty())
  {
    generations = std::max<unsigned>(std::stoul(v), 1);
    std::cout << "Generations: " << generations << '\n';
  }

  unsigned runs(1);
  if (const auto v(cmdl("runs").str()); !v.empty())
  {
    runs = std::max<unsigned>(std::stoul(v), 1);
    std::cout << "Runs: " << runs << '\n';
  }

  ultra::model_measurements<double> threshold;
  if (const auto v(cmdl("threshold").str()); !v.empty())
  {
    if (v.back() == '%')
    {
      const auto v1(v.substr(0, v.size()-1));

      threshold.accuracy = std::clamp<double>(std::stod(v1)/100.0, 0.0, 1.0);
      std::cout << "Threshold: " << *threshold.accuracy * 100.0 << "%\n";
    }
    else
    {
      threshold.fitness = std::stod(v);
      std::cout << "Threshold: " << *threshold.fitness << '\n';
    }
  }

  nogui = cmdl["nogui"];

  for (auto &test : test_collection)
  {
    test.second.generations = generations;
    test.second.runs = runs;
    test.second.threshold = threshold;
  }

  return true;
}

cmdl_result parse_args(int argc, char *argv[])
{
  argh::parser cmdl;

  cmdl.add_param("basename");
  cmdl.add_param("dynamic");
  cmdl.add_param("generations");
  cmdl.add_param("layers");
  cmdl.add_param("population");
  cmdl.add_param("reference");
  cmdl.add_param("refresh");
  cmdl.add_param("runs");
  cmdl.add_param("threshold");
  cmdl.add_param("window");

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

  if (cmd == cmd_monitor && setup_monitor_cmd(cmdl))
    return cmdl_result::monitor;
  else if (cmd == cmd_test  && setup_test_cmd(cmdl))
    return cmdl_result::test;

  return cmdl_result::error;
}

/*********************************************************************
 * MAIN
 ********************************************************************/
void monitor(const imgui_app::program::settings &settings)
{
  std::jthread t_logs(get_logs);

  imgui_app::program prg(settings);
  prg.run(render_monitor);
}

void test(const imgui_app::program::settings &settings)
{
  std::stop_source source;

  const auto test_driver(
    [source](auto test)
    {
      const auto &dataset(test.first);
      ultra::src::problem prob(dataset);
      prob.params.evolution.generations = test.second.generations;

      prob.insert<ultra::real::sin>();
      prob.insert<ultra::real::cos>();
      prob.insert<ultra::real::add>();
      prob.insert<ultra::real::sub>();
      prob.insert<ultra::real::div>();
      prob.insert<ultra::real::mul>();

      ultra::src::search s(prob);

      ultra::search_log sl;
      const std::filesystem::path base_dir(dataset.parent_path());

      sl.dynamic_file_path = build_path(
        base_dir, ultra::dynamic_from_basename(dataset));
      sl.layers_file_path = build_path(
        base_dir, ultra::layers_from_basename(dataset));
      sl.population_file_path = build_path(
        base_dir, ultra::population_from_basename(dataset));
      sl.summary_file_path = build_path(
        base_dir, ultra::summary_from_basename(dataset));

      s.logger(sl).stop_source(source);

      if (test_collection.size() > 1)
        s.tag(dataset.stem());

      return s.run(test.second.runs, test.second.threshold);
    });

  if (test_collection.size() > 1)
    ultra::log::reporting_level = ultra::log::lPAROUT;

  std::vector<std::future<ultra::search_stats<ultra::gp::individual,
                                              double>>> tasks;
  for (const auto &test : test_collection)
    tasks.push_back(std::async(std::launch::async, test_driver, test));

  std::jthread t_summaries(get_summaries);

  if (nogui == false)
  {
    imgui_app::program prg(settings);
    prg.run(render_test);

    source.request_stop();
  }

  const auto task_completed(
    [](const auto &future)
    {
      return future.wait_for(0ms) == std::future_status::ready;
    });

  while (!std::ranges::all_of(tasks, task_completed))
    std::this_thread::sleep_for(100ms);

  t_summaries.request_stop();
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
