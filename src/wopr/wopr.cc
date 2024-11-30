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
#include "utility/log.h"
#include "utility/ts_queue.h"

#include "imgui_app.h"


ultra::search_log slog;

struct dynamic_data
{
  explicit dynamic_data(const std::string &);

  bool new_run {false};
  unsigned generation {0};

  ultra::fitnd fit_best {};
  ultra::fitnd fit_mean {};
  ultra::fitnd fit_std_dev {};
  ultra::fitnd fit_entropy {};
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
        || !(ss >> fit_entropy)
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
  std::vector<float> xs {};
  std::vector<float> fit_best {};
  std::vector<float> fit_mean {};
  std::vector<float> fit_std_dev {};

  [[nodiscard]] std::size_t size() const { return xs.size(); }

  void push_back(const dynamic_data &dd)
  {
    xs.push_back(xs.size());
    fit_best.push_back(dd.fit_best[0]);
    fit_mean.push_back(dd.fit_mean[0]);
    fit_std_dev.push_back(dd.fit_std_dev[0]);
  }
};

struct run_sequence
{
  dynamic_sequence ds {};

  [[nodiscard]] bool empty() const { return ds.xs.empty(); }
  [[nodiscard]] std::size_t size() const { return ds.size(); }

  void push_back(const dynamic_data &dd)
  {
    ds.push_back(dd);
  }
};

std::vector<run_sequence> plot_data;

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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void get_data(std::stop_token stoken)
{
  assert(!slog.dynamic_file_path.empty() || !slog.layers_file_path.empty()
         || !slog.population_file_path.empty());

  std::jthread read_dynamic;
  ultra::ts_queue<std::string> dynamic_buffer;
  if (!slog.dynamic_file_path.empty())
    read_dynamic = std::jthread(read_file, slog.dynamic_file_path,
                                std::ref(dynamic_buffer));

  while (!stoken.stop_requested())
  {
    if (const auto line(dynamic_buffer.try_pop()); line)
      dynamic_queue.push(dynamic_data(*line));
  }
}

void render()
{
  if (slog.dynamic_file_path.empty())
    return;

  ImGui::Begin("Dynamics");

  //if (!ImGui::Button("Pause"))
  //  arena.simulate();

  if (const auto data(dynamic_queue.try_pop()); data)
  {
    if (data->new_run)
    {
      // Skip multiple empty lines.
      if (plot_data.empty() || !plot_data.back().empty())
        plot_data.push_back({});
    }
    else
      plot_data.back().push_back(*data);
  }

  for (std::size_t run(0); run < plot_data.size(); ++run)
  {
    const std::string run_string("Run " + std::to_string(run));
    if (ImGui::CollapsingHeader(run_string.c_str()))
    {
      //ImGui::PlotLines("Fitness", plot_data[run].ds.fit_best.data(),
      //                 plot_data[run].ds.size());
      if (ImPlot::BeginPlot("Fitness"))
      {
        const auto &pdr(plot_data[run]);
        const auto &xs(pdr.ds.xs);

        //ImPlot::SetupAxesLimits(0, plot_data[run].size(),
        //                        plot_data[run].ds.seq_min,
        //                        plot_data[run].ds.seq_max);
        ImPlot::SetNextErrorBarStyle(ImPlot::GetColormapColor(1), 0);
        ImPlot::PlotErrorBars("Line",
                              xs.data(),
                              pdr.ds.fit_mean.data(),
                              pdr.ds.fit_std_dev.data(),
                              xs.size());
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Square);
        ImPlot::PlotLine("Line",
                         xs.data(),
                         pdr.ds.fit_mean.data(),
                         xs.size());
        ImPlot::EndPlot();
      }
    }
  }

  ImGui::End();
}

bool parse_args(int argc, char *argv[])
{
  argh::parser cmdl;
  cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  if (cmdl(1))
  {
    if (std::filesystem::exists(cmdl[1]))
      slog.base_dir = cmdl[1];
    else
    {
      std::cerr << "Data directory doesn't exist\n";
      return false;
    }
  }

  const auto build_path([base_dir = slog.base_dir]
                        (const std::filesystem::path &f,
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
  });

  using namespace ultra;
  slog.dynamic_file_path = build_path(cmdl("dynamic", "").str(),
                                      search_log::default_dynamic_file);
  slog.layers_file_path = build_path(cmdl("layers", "").str(),
                                     search_log::default_layers_file);
  slog.population_file_path = build_path(cmdl("population", "").str(),
                                         search_log::default_population_file);

  std::cout << "Dynamic file path: " << slog.dynamic_file_path
            << "\nLayers file path: " << slog.layers_file_path
            << "\nPopulation file path: " << slog.population_file_path << '\n';

  return true;
}

int main(int argc, char *argv[])
{
  if (!parse_args(argc, argv))
  {
    std::cerr << "Cannot parse command line\n";
    return EXIT_FAILURE;
  }

  if (slog.dynamic_file_path.empty() && slog.layers_file_path.empty()
      && slog.population_file_path.empty())
  {
    std::cerr << "No data file available\n";
    return EXIT_FAILURE;
  }

  std::jthread t_data(get_data);

  imgui_app::program prg{"WOPR"};

  prg.run(render);

  return EXIT_SUCCESS;
}
