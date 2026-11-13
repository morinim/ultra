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

#include "command_line.h"

#include "monitor.h"
#include "results.h"

#include "kernel/search_log.h"

#include "CLI11/CLI11.hpp"

#include <array>
#include <expected>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

using log_file_discovery = std::expected<fs::path, fs::path>;

// - `basename` identifies the test/run request by the user:
//   `wopr monitor /path/to/test` produces `folder = "/path/to"` and
//   `basename = "test"`. It remains empty when the user supply only a
//   directory.
// - `marker` identifies the kind of log being discovered (`dynamic.txt`,
//   `layers.txt`...).
//
// Together they're intended to match filenames such as `test.dynamic.txt`,
// `test.layers.txt`...
[[nodiscard]] log_file_discovery discover_log_file(
  const fs::path &folder, const std::string &basename, const std::string &marker)
{
  if (!basename.empty())
  {
    const auto filename(basename + "." + marker);
    const auto path(folder / filename);
    return fs::is_regular_file(path) ? path : fs::path{};
  }

  fs::path found;
  const auto suffix("." + marker);

  for (const auto &entry : fs::directory_iterator(folder))
  {
    if (!entry.is_regular_file())
      continue;

    if (const auto filename(entry.path().filename().string());
        filename != marker && !filename.ends_with(suffix))
      continue;

    if (!found.empty())
      return std::unexpected(
        fs::path(entry.path()).replace_extension().replace_extension());

    found = entry.path();
  }

  return found;
}

struct monitor_cli
{
  fs::path log_object{"./"};
  fs::path dynamic_file;
  fs::path layers_file;
  fs::path population_file;
  std::optional<int> window;
};

struct run_cli
{
  fs::path test_input{"./"};
  fs::path ref_folder;
  rs::settings defaults;
  unsigned threads{0};
  std::string threshold_str;
  bool nogui{false};
};

struct summary_cli
{
  fs::path dir1;
  fs::path dir2;
};

std::expected<monitor::options, std::string> finish_monitor(
  const monitor_cli &cli, bool imgui_demo)
{
  using namespace ultra;

  monitor::options options;
  fs::path log_folder;
  std::string basename;

  if (fs::is_directory(cli.log_object))
    log_folder = cli.log_object;
  else
  {
    basename = cli.log_object.filename().string();

    log_folder = cli.log_object.parent_path();
    if (log_folder.empty())
      log_folder = "./";
  }

  if (!fs::is_directory(log_folder))
    return std::unexpected(log_folder.string() + " isn't a directory.");

  options.slog.base_dir = log_folder;
  options.slog.summary_file_path = "";

  options.slog.dynamic_file_path = build_path(log_folder, cli.dynamic_file);
  options.slog.layers_file_path = build_path(log_folder, cli.layers_file);
  options.slog.population_file_path =
    build_path(log_folder, cli.population_file);

  const auto discover([&](fs::path &path, const std::string &marker)
  {
    if (!path.empty())
      return true;

    const auto result(discover_log_file(log_folder, basename, marker));
    if (!result)
    {
      std::cerr << "Too many log files in folder; please choose one (e.g."
                << " `wopr monitor " << result.error() << "`).\n";
      return false;
    }

    path = *result;
    return true;
  });

  if (!discover(options.slog.dynamic_file_path,
                search_log::default_dynamic_file)
      || !discover(options.slog.layers_file_path,
                   search_log::default_layers_file)
      || !discover(options.slog.population_file_path,
                   search_log::default_population_file))
    return std::unexpected("Cannot select the log files.");

  const std::array log_paths =
  {
    options.slog.dynamic_file_path, options.slog.layers_file_path,
    options.slog.population_file_path
  };

  if (std::ranges::all_of(log_paths, &fs::path::empty))
    return std::unexpected("No log file available.");

  for (const auto &path : log_paths)
    if (!path.empty() && !fs::is_regular_file(path))
      return std::unexpected(
        "A configured log file (" + path.string() + ") is not available.");

  std::cout << "Dynamic file path: " << options.slog.dynamic_file_path
            << "\nLayers file path: " << options.slog.layers_file_path
            << "\nPopulation file path: " << options.slog.population_file_path
            << '\n';

  if (cli.window)
  {
    options.window = *cli.window;
    std::cout << "Monitoring window: " << options.window << '\n';
  }

  options.imgui_demo = imgui_demo;
  return options;
}

std::expected<rs::summary::options, std::string> finish_summary(
  const summary_cli &cli, bool imgui_demo)
{
  auto collection(
    rs::setup_collection(cli.dir1, cli.dir2, rs::exec_mode::summary));

  if (collection.empty())
    return std::unexpected("Cannot create the results collection.");

  return rs::summary::options{
    .collection = std::move(collection),
    .imgui_demo = imgui_demo
  };
}

std::expected<rs::run::options, std::string> finish_run(
  const run_cli &cli, bool imgui_demo)
{
  auto defaults(cli.defaults);

  if (!cli.threshold_str.empty())
  {
    if (const auto threshold(rs::extract_threshold(cli.threshold_str));
        threshold)
      defaults.threshold = *threshold;
    else
      return std::unexpected(threshold.error());
  }

  auto collection(
    rs::setup_collection(cli.test_input, cli.ref_folder, rs::exec_mode::run,
                         defaults));

  if (collection.empty())
    return std::unexpected("Cannot create the run collection.");

  return rs::run::options{
    .collection = std::move(collection),
    .threads = cli.threads,
    .nogui = cli.nogui,
    .imgui_demo = imgui_demo
  };
}

}  // namespace

std::expected<command, std::string> parse_args(int argc, char *argv[])
{
  CLI::App app;
  app.name("wopr");
  app.description(
R"( _       ___   ___   ___
\ \    // / \ | |_) | |_)
 \_\/\/ \_\_/ |_|   |_| \

GREETINGS PROFESSOR FALKEN.

Please enter your selection:)");

  auto fmt(std::make_shared<CLI::Formatter>());
  fmt->enable_description_formatting(false);
  app.formatter(fmt);
  app.footer("\nSHALL WE PLAY A GAME?\n");
  app.fallthrough();
  app.ignore_case();
  app.require_subcommand(1);

  bool imgui_demo(false);
#if defined(ULTRA_WOPR_GUI)
  app.add_flag("--imguidemo", imgui_demo, "Enable ImGUI demo panel");
#endif

  monitor_cli mon;
  auto *mon_cmd(
    app.add_subcommand("monitor", "Observe a running test in real-time"));
  mon_cmd->add_option("path", mon.log_object,
                      "Directory with search log or test prefix (defaults to ./)");
  mon_cmd->add_option("--dynamic", mon.dynamic_file,
                      "Monitor dynamic log with non-default name");
  mon_cmd->add_option("--layers", mon.layers_file,
                      "Monitor layers log with non-default name");
  mon_cmd->add_option("--population", mon.population_file,
                      "Monitor population log with non-default name");
  mon_cmd->add_option(
    "--window", mon.window,
    "Restrict the monitoring window to the last <nr> generations (0 for full history)")
    ->check(CLI::Range(0, std::numeric_limits<int>::max()));

  run_cli run;
  auto *run_cmd(
    app.add_subcommand("run", "Execute tests on dataset(s)"));
  run_cmd->add_option("path", run.test_input,
                      "Dataset file (.csv) or folder containing datasets (defaults to ./)");
  run_cmd->add_option(
    "--generations", run.defaults.generations,
    "Set the maximum number of generations in a run")
    ->check(CLI::Range(1u, std::numeric_limits<unsigned>::max()));
  run_cmd->add_option(
    "--runs", run.defaults.runs,
    "Perform the specified number of evolutionary runs")
    ->check(CLI::Range(1u, std::numeric_limits<unsigned>::max()));
  run_cmd->add_option(
    "-j,--threads", run.threads,
    "Set the maximum number of concurrent searches (defaults to hardware threads)")
    ->check(CLI::Range(1u, std::numeric_limits<unsigned>::max()));
  run_cmd->add_option("--reference", run.ref_folder,
                      "Specify a directory containing reference results");
  run_cmd->add_option("--threshold", run.threshold_str,
                      "Set the success threshold (accuracy ending in '%' or fitness value)");
  run_cmd->add_flag("--nogui", run.nogui,
                    "Disable the graphical user interface (headless mode)");

  summary_cli sum;
  auto *sum_cmd(
    app.add_subcommand("summary",
                       "Display or compare stats for completed tests"));
  sum_cmd->add_option("dir1", sum.dir1,
                      "First directory containing completed summaries")
    ->required();
  sum_cmd->add_option("dir2", sum.dir2,
                      "Optional second directory for comparison");

  if (argc <= 1)
    return help_command{app.help()};

  try
  {
    app.parse(argc, argv);
  }
  catch (const CLI::CallForHelp &)
  {
    return help_command{app.help()};
  }
  catch (const CLI::ParseError &e)
  {
    return std::unexpected(e.what());
  }

  if (run_cmd->parsed())
    return finish_run(run, imgui_demo);
#if defined(ULTRA_WOPR_GUI)
  if (mon_cmd->parsed())
    return finish_monitor(mon, imgui_demo);
  if (sum_cmd->parsed())
    return finish_summary(sum, imgui_demo);
#else
  if (mon_cmd->parsed())
    return std::unexpected(
      "The `monitor` subcommand requires GUI support (WOPR was compiled in headless mode).");
  if (sum_cmd->parsed())
    return std::unexpected(
      "The `summary` subcommand requires GUI support (WOPR was compiled in headless mode).");
#endif

  return help_command{app.help()};
}

}  // namespace ultra::wopr
