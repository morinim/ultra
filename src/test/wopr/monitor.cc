/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "wopr/html_monitor.h"
#include "wopr/monitor.h"
#include "wopr/monitor_data.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

namespace fs = std::filesystem;

using namespace ultra::wopr;

TEST_SUITE("WOPR MONITOR")
{

TEST_CASE("build_layers_fit_matrix handles single layer edges")
{
  SUBCASE("many individuals")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 4318");
    const auto matrix(build_layers_fit_matrix(ll));
    CHECK(matrix.rows == 1);
    CHECK(matrix.cols == 100);
    CHECK(matrix.ind_max == 4318);
    CHECK(matrix.values.size() == 100);
    CHECK(std::ranges::all_of(matrix.values, [](double v)
    {
      return std::isfinite(v) && v >= -1.0 && v <= -0.23;
    }));
  }

  SUBCASE("single individual (parts == 1)")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 1");
    const auto matrix(build_layers_fit_matrix(ll));
    CHECK(matrix.rows == 1);
    CHECK(matrix.cols == 1);
    CHECK(matrix.ind_max == 1);
    REQUIRE(matrix.values.size() == 1);
    CHECK(matrix.values[0] >= -1.0);
    CHECK(matrix.values[0] <= -0.23);
  }

  SUBCASE("zero individuals")
  {
    const layers_line ll("0 0 0 0 0 0 -0.51 0.16 -1.0 -0.23 0");
    const auto matrix(build_layers_fit_matrix(ll));
    CHECK(matrix.empty());
  }
}

TEST_CASE("build_layers_fit_matrix handles multiple layers")
{
  const layers_line ll(
    "0 0 1.0 0.5 1 2 0.5 0.1 0.3 0.7 50 10 2.0 0.8 1 3 0.8 0.2 0.6 0.9 60");
  const auto matrix(build_layers_fit_matrix(ll));
  CHECK(matrix.rows == 2);
  CHECK(matrix.cols == 60);
  CHECK(matrix.ind_max == 60);
  CHECK(matrix.fit_min == doctest::Approx(0.3));
  CHECK(matrix.fit_max == doctest::Approx(0.9));
  CHECK(matrix.values.size() == 120);
}

TEST_CASE("monitor writes one-shot ECharts HTML from search logs")
{
  const auto temp_dir(fs::temp_directory_path() / "wopr_test_monitor_html");
  fs::remove_all(temp_dir);
  fs::create_directories(temp_dir);

  struct cleanup
  {
    const fs::path &dir;
    ~cleanup()
    {
      std::error_code ec;
      fs::remove_all(dir, ec);
    }
  } guard{temp_dir};

  const auto dyn_path(temp_dir / "dynamic.txt");
  const auto pop_path(temp_dir / "population.txt");
  const auto lay_path(temp_dir / "layers.txt");
  const auto html_path(temp_dir / "monitor.html");

  std::ofstream(dyn_path) << "0 0.9 0.5 0.1 0.1 12 2.0 20 {0:4,1:2} x </script>\n";
  std::ofstream(pop_path) << "0 0.5 10 0.9 5\n";
  std::ofstream(lay_path) << "0 5 2.5 1.0 1 4 0.5 0.1 0.2 0.9 15\n";

  monitor::options opts;
  opts.slog.base_dir = temp_dir;
  opts.slog.dynamic_file_path = dyn_path;
  opts.slog.population_file_path = pop_path;
  opts.slog.layers_file_path = lay_path;
  opts.window = 50;
  opts.html_output = html_path;
  opts.interval = 0;

  CHECK(monitor::start(std::move(opts)));
  REQUIRE(fs::is_regular_file(html_path));

  std::ifstream in(html_path, std::ios::binary);
  const std::string html((std::istreambuf_iterator<char>(in)), {});
  CHECK(html.starts_with("<!DOCTYPE html>"));
  CHECK(html.contains("\"live\":false"));
  CHECK(html.contains("\"window\":50"));
  CHECK(html.contains("\"CT0\":[4]"));
  CHECK(html.contains("\"best_prg\":[\"x \\u003c/script\\u003e\"]"));
  CHECK(html.contains("\"fit\":[0.5,0.9]"));
  CHECK(html.contains("\"ind_max\":15"));
  CHECK(html.contains("echarts"));
}

TEST_CASE("JavaScript monitor renderers configure histogram baseline, tabs and live controls")
{
  if (std::system("command -v gjs >/dev/null 2>&1") != 0)
  {
    MESSAGE("Skipping JS monitor tests: gjs not found");
    return;
  }

  const auto temp_dir(fs::temp_directory_path() / "wopr_test_js_monitor");
  fs::remove_all(temp_dir);
  fs::create_directories(temp_dir);

  struct cleanup
  {
    const fs::path &dir;
    ~cleanup()
    {
      std::error_code ec;
      fs::remove_all(dir, ec);
    }
  } guard{temp_dir};

  monitor_state state;
  state.push(dynamic_data("0 0.9 0.5 0.1 0.1 12 2.0 20 {0:4,1:2} p0"));
  state.push(dynamic_data("1 0.95 0.6 0.08 0.12 14 1.5 22 {0:5,1:3} p1"));
  state.push(dynamic_data(""));
  state.push(dynamic_data("0 0.99 0.7 0.05 0.2 10 1.2 18 {0:6,1:1} p2"));
  state.push(population_line("0 0.5 100 0.9 101"));
  state.push(population_line(""));
  state.push(population_line("0 0.4 50 0.8 55"));
  state.push(layers_line("0 5 2.5 1.0 1 4 0.5 0.1 0.2 0.9 15"));
  state.push(layers_line(""));
  state.push(layers_line("1 6 3.0 1.0 2 5 0.6 0.1 0.3 0.95 20"));

  monitor::options opts;
  opts.slog.dynamic_file_path = temp_dir / "dynamic.txt";
  opts.slog.population_file_path = temp_dir / "population.txt";
  opts.slog.layers_file_path = temp_dir / "layers.txt";
  opts.interval = 2;

  REQUIRE(write_monitor_html(temp_dir / "monitor_oneshot.html",
                             state, opts, false));
  REQUIRE(write_monitor_html(temp_dir / "monitor_live.html",
                             state, opts, true));
  REQUIRE(write_monitor_html(temp_dir / "monitor_final.html",
                             state, opts, true));

  const auto read_html([](const fs::path &path)
  {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
  });
  CHECK(read_html(temp_dir / "monitor_live.html")
        .contains("<script src=\"monitor_live.echarts.min.js\">"));
  CHECK(fs::is_regular_file(temp_dir / "monitor_live.echarts.min.js"));
  CHECK(fs::is_regular_file(temp_dir / "monitor_final.echarts.min.js"));
  CHECK_FALSE(read_html(temp_dir / "monitor_oneshot.html").contains("<script src="));
  REQUIRE(write_monitor_html(temp_dir / "monitor_final.html", state, opts, false));
  CHECK_FALSE(read_html(temp_dir / "monitor_final.html").contains("<script src="));
  CHECK_FALSE(fs::is_regular_file(temp_dir / "monitor_final.echarts.min.js"));
  CHECK(fs::is_regular_file(temp_dir / "monitor_live.echarts.min.js"));

  fs::path script_path("check_monitor.js");
  if (!fs::is_regular_file(script_path))
    script_path = fs::path(__FILE__).parent_path() / "check_monitor.js";
  REQUIRE(fs::is_regular_file(script_path));

  const std::string cmd("gjs \"" + script_path.string() + "\" \""
                        + temp_dir.string() + "\"");
  CHECK(std::system(cmd.c_str()) == 0);
}

TEST_CASE("monitor drains newly appended log entries before writing final report on SIGINT")
{
  using namespace std::chrono_literals;

  const auto temp_dir(fs::temp_directory_path() / "wopr_test_monitor_sigint");
  fs::remove_all(temp_dir);
  fs::create_directories(temp_dir);

  struct cleanup
  {
    const fs::path &dir;
    ~cleanup()
    {
      std::error_code ec;
      fs::remove_all(dir, ec);
    }
  } guard{temp_dir};

  const auto dyn_path(temp_dir / "dynamic.txt");
  const auto pop_path(temp_dir / "population.txt");
  const auto lay_path(temp_dir / "layers.txt");
  const auto html_path(temp_dir / "monitor.html");

  std::ofstream(dyn_path) << "0 0.9 0.5 0.1 0.1 12 2.0 20 {0:4,1:2} p0\n";
  std::ofstream(pop_path) << "0 0.5 10 0.9 5\n";
  std::ofstream(lay_path) << "0 5 2.5 1.0 1 4 0.5 0.1 0.2 0.9 15\n";

  monitor::options opts;
  opts.slog.base_dir = temp_dir;
  opts.slog.dynamic_file_path = dyn_path;
  opts.slog.population_file_path = pop_path;
  opts.slog.layers_file_path = lay_path;
  opts.html_output = html_path;
  opts.interval = 60;

  const auto read_html([&html_path]
  {
    std::ifstream in(html_path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
  });

  std::atomic<bool> finished(false);
  bool ok(false);
  std::thread worker([&opts, &ok, &finished]
  {
    ok = monitor::start(std::move(opts));
    finished = true;
  });
  struct worker_guard
  {
    std::thread &thread;
    std::atomic<bool> &done;
    ~worker_guard()
    {
      if (!done)
        std::raise(SIGINT);
      if (thread.joinable())
        thread.join();
    }
  } worker_cleanup{worker, finished};

  const auto deadline(std::chrono::steady_clock::now() + 5s);
  bool initial_written(false);
  while (std::chrono::steady_clock::now() < deadline)
  {
    if (fs::is_regular_file(html_path)
        && read_html().contains("\"live\":true"))
    {
      initial_written = true;
      break;
    }
    std::this_thread::sleep_for(10ms);
  }
  REQUIRE(initial_written);

  std::ofstream(dyn_path, std::ios::app)
    << "1 0.95 0.6 0.08 0.12 14 1.5 22 {0:5,1:3} p1\n";
  std::ofstream(pop_path, std::ios::app) << "1 0.6 12 0.95 8\n";
  std::ofstream(lay_path, std::ios::app)
    << "1 5 2.6 1.0 1 4 0.6 0.1 0.3 0.95 18\n";

  REQUIRE(std::raise(SIGINT) == 0);
  worker.join();

  CHECK(ok);
  const auto final_html(read_html());
  CHECK(final_html.contains("\"live\":false"));
  CHECK(final_html.contains("\"xs\":[0,1]"));
  CHECK(final_html.contains("\"best_prg\":[\"p0\",\"p1\"]"));
  CHECK(final_html.contains("\"generation\":1"));
  CHECK(final_html.contains("\"ind_max\":18"));
  CHECK_FALSE(fs::is_regular_file(temp_dir / "monitor.echarts.min.js"));
}

}  // TEST_SUITE("WOPR MONITOR")
