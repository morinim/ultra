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
#include "utility/misc.h"

#include "argh/argh.h"

#include <algorithm>
#include <cctype>
#include <expected>
#include <iostream>
#include <iterator>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

using log_file_discovery = std::expected<fs::path, fs::path>;

[[nodiscard]] log_file_discovery discover_log_file(
  const fs::path &folder, std::string_view basename, std::string_view marker)
{
  fs::path found;

  for (const auto &entry : fs::directory_iterator(folder))
  {
    if (!entry.is_regular_file()
        || !ultra::iequals(entry.path().extension(), ".txt"))
      continue;

    const std::string filename(entry.path().filename().string());
    if (filename.find(marker) == std::string::npos
        || (!basename.empty() && filename.find(basename) == std::string::npos))
      continue;

    if (!found.empty())
      return std::unexpected(
        fs::path(entry.path()).replace_extension().replace_extension());

    found = entry.path();
  }

  return found;
}

}  // namespace

[[nodiscard]] ultra::model_measurements<double> extract_threshold(
  const std::string &txt)
{
  ultra::model_measurements<double> threshold;

  if (!txt.empty())
  {
    if (txt.back() == '%')
    {
      const auto v(txt.substr(0, txt.size()-1));
      threshold.accuracy = std::clamp<double>(std::stod(v)/100.0, 0.0, 1.0);
    }
    else
      threshold.fitness = std::stod(txt);
  }

  return threshold;
}

rs::settings rs::read_settings(const fs::path &test_fn,
                               const settings &defaults)
{
  assert(test_fn.extension() == ".csv");

  const auto settings_fn(fs::path(test_fn).replace_extension(".xml"));
  if (!fs::exists(settings_fn))
    return defaults;

  tinyxml2::XMLDocument doc;
  if (const auto result(doc.LoadFile(settings_fn.c_str()));
      result != tinyxml2::XML_SUCCESS)
  {
    std::cerr << "Cannot open settings for " << test_fn << ".\n";
    return defaults;
  }

  settings ret(defaults);

  tinyxml2::XMLConstHandle handle(&doc);
  const auto h_ultra(handle.FirstChildElement("ultra"));

  const auto h_search(h_ultra.FirstChildElement("search"));
  if (const auto *e = h_search.FirstChildElement("generations").ToElement())
    ret.generations = e->UnsignedText(ret.generations);

  if (const auto *e = h_search.FirstChildElement("runs").ToElement())
    ret.runs = e->UnsignedText(ret.runs);

  if (const auto *e = h_search.FirstChildElement("threshold").ToElement())
  {
    if (const char *text = e->GetText())
      ret.threshold = extract_threshold(std::string(text));
  }

  const auto h_dataset(h_ultra.FirstChildElement("dataset"));
  if (const auto *e = h_dataset.FirstChildElement("output_index").ToElement())
    if (const char *text = e->GetText())
    {
      if (ultra::iequals(text, "last"))
        ret.params.output_index = ultra::src::dataframe::params::index::back;
      else if (unsigned output_index;
               e->QueryUnsignedText(&output_index) == tinyxml2::XML_SUCCESS)
        ret.params.output_index = output_index;
    }

  return ret;
}
void cmdl_usage()
{
  std::cout
    << R"( _       ___   ___   ___)" "\n"
       R"(\ \    // / \ | |_) | |_))" "\n"
       R"( \_\/\/ \_\_/ |_|   |_| \)"
       "\n\n"
       "GREETINGS PROFESSOR FALKEN.\n"
       "\n"
       "Please enter your selection:\n"
       "\n"
  "> wopr monitor [path]\n"
  "\n"
  "  OBSERVE A RUNNING TEST IN REAL-TIME\n"
  "  The path must point to a directory containing a search log produced by\n"
  "  ULTRA. If omitted, the current working directory is used.\n"
  "  Omit the file extension when specifying a test path (e.g. use\n"
  "  \"/path/test\" instead of \"/path/test.csv\").\n"
  "\n"
  "  Available switches:\n"
  "\n"
  "  --dynamic    <filepath>\n"
  "  --layers     <filepath>\n"
  "  --population <filepath>\n"
  "      Monitor files with non-default names.\n"
  "  --window <nr>\n"
  "      Restrict the monitoring window to the last `nr` generations.\n"
  "\n"
  "-------------------------------------------------------------------\n"
  "> wopr run [path]\n"
  "\n"
  "  EXECUTE TESTS ON THE SPECIFIED DATASET(s)\n"
  "  The argument must be a folder containing at least one .csv dataset\n"
  "  (and optionally a config file), or a specific dataset file. If omitted,\n"
  "  the current directory is used.\n"
  "\n"
  "  Available switches:\n"
  "\n"
  "  --generations <nr>\n"
  "      Set the maximum number of generations in a run.\n"
  "  --nogui\n"
  "      Disable the graphical user interface (headless mode).\n"
  "  --reference <directory>\n"
  "      Specify a directory containing reference results.\n"
  "  --runs <nr>\n"
  "      Perform the specified number of evolutionary runs.\n"
  "  --threshold <val>\n"
  "      Set the success threshold. Values ending in '%' are treated as\n"
  "      accuracy; otherwise, as fitness value.\n"
  "\n"
  "-------------------------------------------------------------------\n"
  "> wopr summary <directory> [directory]\n"
  "\n"
  "  DISPLAY OR COMPARE STATS FOR COMPLETED TESTS\n"
  "  Displays a high-level overview of the first directory. If a second\n"
  "  directory is provided, a comparison is performed.\n"
  "\n"
  "--help\n"
  "    Display this help screen.\n"
  "--imguidemo\n"
  "    Enable ImGUI demo panel.\n"
  "\n"
  "SHALL WE PLAY A GAME?\n\n";
}

fs::path build_path(fs::path base_dir, fs::path f,
                    const std::string &default_filename)
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
rs::collection_t rs::setup_collection(fs::path in1, fs::path in2, exec_mode m,
                                      const settings &defaults)
{
  using namespace ultra;

  if (in1.empty())
    in1 = "./";

  if (in1 == in2)
  {
    std::cerr << "Same file or directory for comparison.\n";
    return {};
  }

  if (!in2.empty() && !fs::is_directory(in2))
  {
    std::cerr << in2 << " isn't a directory.\n";
    return {};
  }

  collection_t ret;

  const auto check_and_insert([&m, &in2, &ret, &defaults](const auto &path)
  {
    const auto get_reference([&in2](const fs::path &base)
    {
      if (!in2.empty())
      {
        if (const auto ref(in2 / summary_from_basename(base.filename()));
            fs::exists(ref))
        {
          std::cout << "\n  reference: " << ref << '\n';

          try
          {
            return summary_data(ref);
          }
          catch (const std::invalid_argument &e)
          {
            std::cerr << e.what() << '\n';
          }
        }
      }
      else
        std::cout << "\n  no reference\n";

      return summary_data();
    });

    const auto show_settings([](std::string_view name, const settings &ts)
    {
      std::cout << "Settings for " << name
                << "\n  Runs: " << ts.runs
                << "\n  Generations: " << ts.generations
                << "\n  Threshold:";

      if (ts.threshold.accuracy || ts.threshold.fitness)
      {
        if (ts.threshold.accuracy)
          std::cout << ' ' << *ts.threshold.accuracy * 100.0 << '%';
        if (ts.threshold.fitness)
          std::cout << ' ' << *ts.threshold.fitness;
      }
      else
        std::cout << " none";
      std::cout << '\n';
    });

    const auto ext(path.extension());

    if (m == exec_mode::run && ultra::iequals(ext, ".csv"))
    {
      const auto sum(summary_from_basename(path));

      std::cout << "Basename: " << path
                << "\n  first summary: " << sum;

      const auto ref(get_reference(path));

      const rs::data d(path, sum, read_settings(path, defaults), ref);

      ret.emplace_back(path.stem(), d);

      show_settings(ret.back().first, ret.back().second.conf);
      return true;
    }

    if (m == exec_mode::summary && ultra::iequals(ext, ".xml")
        && path.stem().string().ends_with(".summary"))
    {
      const auto basename(basename_from_summary(path));

      std::cout << "Basename: " << basename
                << "\n  first summary: " << path;

      const auto ref(get_reference(basename));

      const rs::data d(basename, path, {}, ref);

      ret.emplace_back(basename.stem(), d);
      return true;
    }

    return false;
  });

  if (fs::is_directory(in1))
  {
    std::vector<fs::path> sf;

    for (const auto &entry : fs::directory_iterator(in1))
    {
      if (!entry.is_regular_file())
        continue;

      const auto &p(entry.path());

      if (const auto ext(p.extension());
          ultra::iequals(ext, ".csv") || ultra::iequals(ext, ".xml"))
        sf.push_back(p);
    }

    std::ranges::sort(sf, {}, &fs::path::filename);

    for (const auto &entry : sf)
      check_and_insert(entry);
  }
  else if (fs::exists(in1))
    check_and_insert(in1);
  else
  {
    std::cerr << in1 << " isn't a valid input.\n";
    return {};
  }

  if (ret.empty())
  {
    std::cerr << "No dataset available.\n";
    return {};
  }

  std::cout << "Datasets:";
  for (const auto &ds : ret)
    std::cout << ' ' << ds.first;
  std::cout << '\n';

  return ret;
}

namespace
{

std::expected<monitor::options, std::string>
parse_monitor(const argh::parser &cmdl)
{
  using namespace ultra;

  monitor::options options;
  const auto &pos_args(cmdl.pos_args());

  fs::path log_object(pos_args.size() <= 2 ? "./" : pos_args[2]);

  fs::path log_folder;
  std::string basename;

  if (fs::is_directory(log_object))
    log_folder = log_object;
  else
  {
    basename = log_object.filename();

    log_folder = log_object.parent_path();
    if (log_folder.empty())
      log_folder = "./";
  }

  if (!fs::is_directory(log_folder))
  {
    return std::unexpected(log_folder.string() + " isn't a directory.");
  }

  options.slog.base_dir = log_folder;
  options.slog.summary_file_path = "";

  options.slog.dynamic_file_path =
    build_path(log_folder, cmdl("dynamic", "").str());
  options.slog.layers_file_path =
    build_path(log_folder, cmdl("layers", "").str());
  options.slog.population_file_path =
    build_path(log_folder, cmdl("population", "").str());

  const auto discover([&](fs::path &path, std::string_view marker)
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

  const std::vector log_vect =
  {
    options.slog.dynamic_file_path, options.slog.layers_file_path,
    options.slog.population_file_path
  };

  bool all_empty(true);
  for (const auto &path : log_vect)
  {
    if (!path.empty())
      all_empty = false;

    if (!path.empty() && !fs::is_regular_file(path))
      return std::unexpected(
        "A configured log file (" + path.string() + ") is not available.");
  }

  if (all_empty)
    return std::unexpected("No log file available.");

  std::cout << "Dynamic file path: " << options.slog.dynamic_file_path
            << "\nLayers file path: " << options.slog.layers_file_path
            << "\nPopulation file path: " << options.slog.population_file_path
            << '\n';


  if (const auto v(cmdl("window").str()); !v.empty())
  {
    try
    {
      options.window = std::stoi(v);
      std::cout << "Monitoring window: " << options.window << '\n';
    }
    catch (...)
    {
      return std::unexpected("Wrong value for monitoring window.");
    }
  }

  options.imgui_demo = cmdl["imguidemo"];
  return options;
}

std::expected<rs::summary::options, std::string>
parse_summary(const argh::parser &cmdl)
{
  const auto &pos_args(cmdl.pos_args());

  if (pos_args.size() <= 2)
    return std::unexpected("At least one directory must be specified.");

  const fs::path dir1(pos_args.size() <= 2 ? "./" : pos_args[2]);
  const fs::path dir2(pos_args.size() <= 3 ? "" : pos_args[3]);

  auto collection(rs::setup_collection(dir1, dir2, rs::exec_mode::summary));

  if (collection.empty())
    return std::unexpected("Cannot create the results collection.");

  return rs::summary::options{
    .collection = std::move(collection),
    .imgui_demo = cmdl["imguidemo"]
  };
}

std::expected<rs::run::options, std::string>
parse_run(const argh::parser &cmdl)
{
  const auto &pos_args(cmdl.pos_args());

  for (const auto &a : pos_args)
    std::cout << a << std::endl;

  const fs::path test_input(pos_args.size() <= 2 ? "./" : pos_args[2]);
  const fs::path ref_folder(cmdl("reference", "").str());
  rs::settings defaults;

  try
  {
    if (const auto v(cmdl("generations").str()); !v.empty())
    {
      defaults.generations = std::max<unsigned>(std::stoul(v), 1);
      std::cout << "Generations: " << defaults.generations << '\n';
    }

    if (const auto v(cmdl("runs").str()); !v.empty())
    {
      defaults.runs = std::max<unsigned>(std::stoul(v), 1);
      std::cout << "Runs: " << defaults.runs << '\n';
    }

    if (const auto v(cmdl("threshold").str()); !v.empty())
      defaults.threshold = extract_threshold(v);
  }
  catch (const std::exception &)
  {
    return std::unexpected("Invalid run option value.");
  }

  auto collection(
    rs::setup_collection(test_input, ref_folder, rs::exec_mode::run, defaults));

  if (collection.empty())
    return std::unexpected("Cannot create the run collection.");

  return rs::run::options{
    .collection = std::move(collection),
    .nogui = cmdl["nogui"],
    .imgui_demo = cmdl["imguidemo"]
  };
}

}  // namespace

std::expected<command, std::string> parse_args(int argc, char *argv[])
{
  argh::parser cmdl;

  cmdl.add_param("basename");
  cmdl.add_param("dynamic");
  cmdl.add_param("generations");
  cmdl.add_param("layers");
  cmdl.add_param("population");
  cmdl.add_param("reference");
  cmdl.add_param("runs");
  cmdl.add_param("threshold");
  cmdl.add_param("window");

  cmdl.parse(argc, argv);

  const std::string cmd_monitor("monitor");
  const std::string cmd_run("run");
  const std::string cmd_summary("summary");
  const std::set<std::string> cmds({cmd_monitor, cmd_run, cmd_summary});

  const auto &pos_args(cmdl.pos_args());

  if (pos_args.size() <= 1 || cmdl[{"-h", "--help"}])
    return help_command{};

  std::string cmd;
  std::ranges::transform(pos_args[1], std::back_inserter(cmd),
                         [](unsigned char c){ return std::tolower(c); });

  if (!cmds.contains(cmd))
    return std::unexpected("Unknown command `" + cmd + "`.");

  if (cmd == cmd_summary)
    return parse_summary(cmdl)
      .transform([](auto options) -> command { return options; });
  if (cmd == cmd_monitor)
    return parse_monitor(cmdl)
      .transform([](auto options) -> command { return options; });

  return parse_run(cmdl)
    .transform([](auto options) -> command { return options; });
}

}  // namespace ultra::wopr
