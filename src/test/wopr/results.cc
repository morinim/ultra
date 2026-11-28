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

#include "wopr/command_line.h"
#include "wopr/html_report.h"
#include "wopr/html_shell.h"
#include "wopr/results.h"

#include "tinyxml2/tinyxml2.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace fs = std::filesystem;

namespace
{

using namespace std::chrono_literals;
using namespace ultra::wopr;

struct temp_dir_guard
{
  explicit temp_dir_guard(std::string_view name)
    : path(fs::temp_directory_path() / name)
  {
    fs::remove_all(path);
    fs::create_directories(path);
  }

  ~temp_dir_guard()
  {
    std::error_code ec;
    for (const auto &p : protected_files)
      fs::permissions(p, fs::perms::owner_all, fs::perm_options::replace, ec);
    fs::remove_all(path, ec);
  }

  fs::path path;
  std::vector<fs::path> protected_files {};
};

[[nodiscard]] std::string read_text_file(const fs::path &path)
{
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), {}};
}

[[nodiscard]] rs::collection_t make_collection(
  bool with_reference = true, bool with_threshold = false)
{
  summary_data reference;
  reference.runs = with_reference ? 20 : 0;
  reference.success_rate = 0.5;
  reference.elapsed_time = 90s;

  rs::settings settings;
  settings.runs = 20;
  if (with_threshold)
    settings.threshold.fitness = 0.0;

  rs::collection_t collection;
  collection.push_back({.name = "Measured", .conf = settings, .reference = reference});
  collection.back().current.runs = 10;
  collection.back().current.success_rate = 0.2;
  collection.back().current.elapsed_time = 12500ms;
  collection.push_back({.name = "Pending"});
  collection.push_back({.name = "Zero", .reference = reference});
  collection.back().current.runs = 5;
  return collection;
}

}  // namespace

TEST_SUITE("WOPR RESULTS")
{

TEST_CASE("Dataset activity is shared without changing earlier snapshots")
{
  rs::entry entry{.activity = std::make_shared<rs::dataset_activity>()};
  const auto copy(entry);
  const auto queued(entry.activity->status());
  CHECK(queued.state == rs::task_state::queued);

  std::jthread worker([&copy]
  {
    copy.activity->set(rs::task_state::running);
    copy.activity->set(rs::task_state::failed, "Cannot read dataset");
  });
  worker.join();

  const auto failed(entry.activity->status());
  CHECK(failed.state == rs::task_state::failed);
  CHECK(failed.error == "Cannot read dataset");
  CHECK(queued.state == rs::task_state::queued);
  CHECK(queued.error.empty());
}

TEST_CASE("Failed dataset searches publish their error")
{
  const auto activity(std::make_shared<rs::dataset_activity>());
  rs::run::options options;
  options.collection.push_back({.dataset = "missing-wopr-dataset/data.csv",
                                .activity = activity});
  CHECK_FALSE(rs::run::start(std::move(options)));
  const auto status(activity->status());
  CHECK(status.state == rs::task_state::failed);
  CHECK_FALSE(status.error.empty());
}

TEST_CASE("build_path constructs and normalizes paths correctly")
{
  CHECK(build_path("", "test.txt") == "./test.txt");
  CHECK(build_path("dir", "test.txt") == "dir/test.txt");
  CHECK(build_path("dir", "/abs/test.txt") == "/abs/test.txt");
  CHECK(build_path("dir", "").empty());
  CHECK(build_path("", "").empty());
}

TEST_CASE("make_search_log constructs search log paths from dataset")
{
  SUBCASE("default summary path")
  {
    const auto sl(rs::make_search_log("data/test.csv"));
    CHECK(sl.dynamic_file_path == "data/test.dynamic.txt");
    CHECK(sl.layers_file_path == "data/test.layers.txt");
    CHECK(sl.population_file_path == "data/test.population.txt");
    CHECK(sl.summary_file_path == "data/test.summary.xml");
  }

  SUBCASE("explicit summary path")
  {
    const auto sl(rs::make_search_log("data/test.csv", "out/custom.xml"));
    CHECK(sl.dynamic_file_path == "data/test.dynamic.txt");
    CHECK(sl.layers_file_path == "data/test.layers.txt");
    CHECK(sl.population_file_path == "data/test.population.txt");
    CHECK(sl.summary_file_path == "out/custom.xml");
  }
}

TEST_CASE("Summary files require valid XML and matching checksums")
{
  const fs::path path("wopr_summary_check.xml");
  struct cleanup
  {
    const fs::path &path;
    ~cleanup()
    {
      std::error_code error;
      fs::remove(path, error);
    }
  } guard{path};

  std::string xml(ultra::crc32::embed_xml_signature(
    "<ultra><summary><runs>2</runs><best><code>x &lt; y</code></best>"
    "</summary><checksum>00000000</checksum></ultra>"));
  bool valid(false);
  bool missing(false);

  SUBCASE("Valid summary") { valid = true; }
  SUBCASE("Tampered summary") { xml.replace(xml.find("<runs>2"), 7, "<runs>3"); }
  SUBCASE("Missing checksum") { xml = "<ultra><summary/></ultra>"; }
  SUBCASE("Malformed XML with a valid checksum")
  {
    xml = ultra::crc32::embed_xml_signature(
      "<ultra><summary></ultra><checksum>00000000</checksum>");
  }
  SUBCASE("Empty file") { xml.clear(); }
  SUBCASE("Missing file") { missing = true; }

  if (missing)
    fs::remove(path);
  else
  {
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out);
    out << xml;
    REQUIRE(out);
  }

  tinyxml2::XMLDocument doc;
  CHECK(summary_data::check(path, doc) == valid);
  if (valid)
  {
    const summary_data summary(doc);
    CHECK(summary.runs == 2);
    CHECK(summary.best_prg == "x < y");
  }
}

TEST_CASE("Missing success rates remain distinct from measured zero")
{
  auto collection(make_collection());
  const results_snapshot snapshot(collection);

  REQUIRE(snapshot.datasets.size() == 3);
  CHECK(snapshot.datasets[0].current.success_rate == doctest::Approx(20.0));
  CHECK(std::isnan(snapshot.datasets[1].current.success_rate));
  CHECK(snapshot.datasets[2].current.success_rate == doctest::Approx(0.0));
  CHECK(snapshot.datasets[0].reference.success_rate == doctest::Approx(50.0));
  CHECK(std::isnan(snapshot.datasets[1].reference.success_rate));
  CHECK(snapshot.datasets[2].reference.success_rate == doctest::Approx(50.0));

  collection[1].current.runs = 1;
  const results_snapshot updated(collection);
  CHECK(updated.datasets[1].current.success_rate == doctest::Approx(0.0));
}

TEST_CASE("Runs without success thresholds use time per completed run")
{
  const auto check([](rs::exec_mode mode, bool with_threshold)
  {
    const auto collection(make_collection(true, with_threshold));
    const results_snapshot snapshot(collection, mode);

    CHECK(snapshot.show_success_rate
          == (mode == rs::exec_mode::summary || with_threshold));
    REQUIRE(snapshot.datasets.size() == 3);
    CHECK(snapshot.datasets[0].current.time_per_run == doctest::Approx(1.25));
    CHECK(std::isnan(snapshot.datasets[1].current.time_per_run));
    CHECK(snapshot.datasets[2].current.time_per_run == doctest::Approx(0.0));
    CHECK(snapshot.datasets[0].reference.time_per_run == doctest::Approx(4.5));
    CHECK(std::isnan(snapshot.datasets[1].reference.time_per_run));

    if (mode == rs::exec_mode::run && with_threshold)
    {
      CHECK(snapshot.datasets[0].has_threshold);
      CHECK_FALSE(snapshot.datasets[2].has_threshold);
    }
  });

  check(rs::exec_mode::run, false);
  check(rs::exec_mode::run, true);
  check(rs::exec_mode::summary, false);

  rs::settings settings;
  settings.threshold.accuracy = 0.9;
  rs::collection_t collection;
  collection.push_back({.name = "Accuracy", .conf = settings});
  CHECK(results_snapshot(collection, rs::exec_mode::run).show_success_rate);
}

TEST_CASE("Remaining time estimates use each dataset's planned runs")
{
  rs::settings settings;
  settings.runs = 20;
  summary_data reference;
  reference.runs = 100;
  reference.elapsed_time = 100h;
  rs::collection_t collection;
  collection.push_back({.name = "Fast", .conf = settings, .reference = reference});
  collection.push_back({.name = "Slow", .conf = settings});
  collection.push_back({.name = "Pending", .conf = settings});
  collection[0].current.runs = 10;
  collection[0].current.elapsed_time = 12500ms;
  collection[1].current.runs = 5;
  collection[1].current.elapsed_time = 2h;
  const results_snapshot snapshot(collection, rs::exec_mode::run);

  REQUIRE(estimated_time_remaining(snapshot.datasets[0]));
  CHECK(*estimated_time_remaining(snapshot.datasets[0]) == doctest::Approx(12.5));
  CHECK(remaining_time_label(snapshot.datasets[0]) == "~13s");
  CHECK(remaining_time_label(snapshot.datasets[1]) == "~6h 0m 0s");
  CHECK_FALSE(estimated_time_remaining(snapshot.datasets[2]));
  CHECK(remaining_time_label(snapshot.datasets[2]) == "Pending");

  collection[0].current.runs = 15;
  collection[0].current.elapsed_time = 3min;
  const results_snapshot updated(collection, rs::exec_mode::run);
  CHECK(remaining_time_label(updated.datasets[0]) == "~1m 0s");

  for (const unsigned runs : {20u, 21u})
  {
    collection[0].current.runs = runs;
    const results_snapshot completed(collection, rs::exec_mode::run);
    REQUIRE(estimated_time_remaining(completed.datasets[0]));
    CHECK(*estimated_time_remaining(completed.datasets[0]) == doctest::Approx(0.0));
    CHECK(remaining_time_label(completed.datasets[0]) == "Complete");
  }
}

TEST_CASE("JavaScript tooltip renderers reflect comparisons, timings and missing results")
{
  if (std::system("command -v gjs >/dev/null 2>&1") != 0)
  {
    MESSAGE("Skipping JS tooltip tests: gjs not found");
    return;
  }

  const temp_dir_guard temp_dir("wopr_test_js_tooltips");
  const auto write_case([&temp_dir](std::string_view name,
                                     const rs::collection_t &collection,
                                     rs::exec_mode mode)
  {
    const results_snapshot snapshot(collection, mode);
    const auto html(render_html_report(snapshot));
    const auto path(temp_dir.path / (std::string(name) + ".html"));
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out);
    out << html;
  });

  auto measured_coll(make_collection());
  write_case("sr_default", measured_coll, rs::exec_mode::summary);
  REQUIRE(write_html_report(temp_dir.path / "sr_external.html",
                             results_snapshot(measured_coll), true, 3));

  measured_coll[0].current.success_rate = 0.75;
  write_case("sr_improved", measured_coll, rs::exec_mode::summary);

  measured_coll[0].current.runs = 0;
  write_case("sr_awaiting", measured_coll, rs::exec_mode::summary);

  write_case("sr_missing_thresh", make_collection(true, true),
             rs::exec_mode::run);

  write_case("tpr_default", make_collection(), rs::exec_mode::run);

  auto single_run(make_collection());
  single_run.resize(1);
  single_run[0].conf.runs = 1;
  single_run[0].current.runs = 1;
  single_run[0].current.fit_mean = {0.8};
  single_run[0].current.fit_std_dev = {0.1};
  single_run[0].current.best_fit = {0.9};
  single_run[0].reference = single_run[0].current;
  write_case("fitness_single_run", single_run, rs::exec_mode::run);

  rs::settings rem_settings;
  rem_settings.runs = 20;
  rs::collection_t rem_coll;
  rem_coll.push_back({.name = "Fast", .conf = rem_settings});
  rem_coll[0].current.runs = 10;
  rem_coll[0].current.elapsed_time = 12500ms;
  write_case("tpr_remaining", rem_coll, rs::exec_mode::run);

  rem_coll[0].current.runs = 20;
  write_case("tpr_done20", rem_coll, rs::exec_mode::run);

  rem_coll[0].current.runs = 21;
  write_case("tpr_done21", rem_coll, rs::exec_mode::run);

  auto elite_coll(make_collection());
  elite_coll.resize(1);
  elite_coll[0].conf.runs = 300;
  for (unsigned k(0); k < 15; ++k)
  {
    const double fit(-static_cast<double>(k) * 0.01);
    const double acc(0.99 - static_cast<double>(k) * 0.01);
    elite_coll[0].current.elite.push_back({k + 1, {{fit}, acc}});
    elite_coll[0].reference.elite.push_back({k + 100, {{fit - 0.02}, acc}});
  }
  write_case("elite_cap", elite_coll, rs::exec_mode::summary);

  rs::settings no_ref_settings;
  no_ref_settings.runs = 5;
  rs::collection_t no_ref_coll;
  no_ref_coll.push_back({.name = "Single", .conf = no_ref_settings});
  no_ref_coll[0].current.runs = 2;
  no_ref_coll[0].current.elapsed_time = 200ms;
  write_case("runs_no_ref", no_ref_coll, rs::exec_mode::run);

  {
    rs::settings thr_settings;
    thr_settings.runs = 10;
    thr_settings.threshold.accuracy = 0.9;
    rs::collection_t live_coll;

    auto act_fail(std::make_shared<rs::dataset_activity>());
    act_fail->set(rs::task_state::failed, "boom");
    live_coll.push_back({
      .name = "bad",
      .conf = thr_settings,
      .activity = act_fail
    });
    live_coll[0].reference.runs = 10;
    live_coll[0].reference.best_fit = {0.0};
    live_coll[0].reference.best_prg = "ref_p";

    auto act_run(std::make_shared<rs::dataset_activity>());
    act_run->set(rs::task_state::running);
    live_coll.push_back({
      .name = "nohit",
      .conf = thr_settings,
      .activity = act_run
    });
    live_coll[1].current.runs = 5;
    live_coll[1].current.success_rate = 0.0;
    live_coll[1].current.best_fit = {-0.5};
    live_coll[1].current.fit_mean = {-0.6};
    live_coll[1].current.fit_std_dev = {0.05};
    live_coll[1].current.best_prg = "x + 1";
    live_coll[1].reference.runs = 10;
    live_coll[1].reference.success_rate = 0.8;
    live_coll[1].reference.best_fit = {-0.1};
    live_coll[1].reference.fit_mean = {-0.2};
    live_coll[1].reference.fit_std_dev = {0.02};
    live_coll[1].reference.best_prg = "x";

    const results_snapshot live_snap(live_coll, rs::exec_mode::run);
    std::ofstream out(temp_dir.path / "live_ui.html", std::ios::binary);
    REQUIRE(out);
    out << render_html_report(live_snap, true, 3);

    rs::collection_t queued_coll;
    auto act_queued(std::make_shared<rs::dataset_activity>());
    queued_coll.push_back({
      .name = "queued_only",
      .conf = thr_settings,
      .activity = act_queued
    });
    const results_snapshot queued_snap(queued_coll, rs::exec_mode::run);
    std::ofstream q_out(temp_dir.path / "queued_only.html", std::ios::binary);
    REQUIRE(q_out);
    q_out << render_html_report(queued_snap, true, 3);
  }

  fs::path script_path("check_tooltips.js");
  if (!fs::is_regular_file(script_path))
    script_path = fs::path(__FILE__).parent_path() / "check_tooltips.js";
  REQUIRE(fs::is_regular_file(script_path));

  const std::string cmd("gjs \"" + script_path.string() + "\" \""
                        + temp_dir.path.string() + "\"");
  CHECK(std::system(cmd.c_str()) == 0);
}

TEST_CASE("read_settings with common_random_numbers")
{
  const auto temp_dir(fs::temp_directory_path() / "wopr_test_crn");
  fs::create_directories(temp_dir);

  const auto csv_path(temp_dir / "test.csv");
  const auto xml_path(temp_dir / "test.xml");

  std::ofstream(csv_path) << "x,y\n1,2\n";
  std::ofstream(xml_path) << R"(<ultra>
  <common_random_numbers>true</common_random_numbers>
  <search>
    <generations>50</generations>
    <runs>5</runs>
  </search>
</ultra>)";

  const auto conf(rs::read_settings(csv_path));
  CHECK(conf.generations == 50);
  CHECK(conf.runs == 5);
  CHECK(conf.common_random_numbers == true);

  fs::remove_all(temp_dir);
}

TEST_CASE("HTML report serializes snapshot and escapes unsafe strings")
{
  auto collection(make_collection(true, true));
  collection[0].activity = std::make_shared<rs::dataset_activity>();
  collection[0].activity->set(rs::task_state::finished);
  collection[0].current.best_prg = "x < 1 </script> \"quoted\"";
  collection[0].current.fit_mean = {0.75};
  collection[0].current.fit_std_dev = {0.125};
  collection[0].current.best_fit = {0.95};
  collection[0].current.elite = {{3u, {{0.95}, 0.875}}};

  collection[1].activity = std::make_shared<rs::dataset_activity>();
  collection[1].activity->set(rs::task_state::failed,
                               "Bad </script> & \"error\"");

  const results_snapshot snapshot(collection, rs::exec_mode::run);
  const auto html(render_html_report(snapshot));

  const std::string_view marker("<script id=\"wopr-data\" type=\"application/json\">");
  const auto data_start(html.find(marker));
  REQUIRE(data_start != std::string::npos);
  const auto payload_start(data_start + marker.size());
  const auto payload_end(html.find("</script>", payload_start));
  REQUIRE(payload_end != std::string::npos);
  const std::string_view payload(
    html.data() + payload_start, payload_end - payload_start);

  CHECK(html.starts_with("<!DOCTYPE html>"));
  CHECK(html.contains(".panel[hidden] {\n  display: none;\n}"));
  CHECK(payload.contains("\"name\":\"Measured\""));
  CHECK(payload.contains("\"name\":\"Pending\""));
  CHECK(payload.contains("\"name\":\"Zero\""));
  CHECK(payload.contains("\"live\":false"));
  CHECK(payload.contains("\"references_available\":true"));
  CHECK(payload.contains("\"show_success_rate\":true"));
  CHECK(payload.contains("\"status\":\"Finished\""));
  CHECK(payload.contains("\"status\":\"Failed\""));
  CHECK(html.contains("echarts"));
  CHECK(payload.contains("null"));
  CHECK_FALSE(payload.contains("nan"));
  CHECK_FALSE(payload.contains("NaN"));
  CHECK_FALSE(payload.contains("x < 1 </script>"));
  CHECK(payload.contains("\\u003c/script\\u003e"));
}

TEST_CASE("Live HTML uses a per-report companion script and removes it on final write")
{
  const temp_dir_guard temp_dir("wopr_test_companion_script");
  const auto html_path(temp_dir.path / "report.html");
  const auto second_html_path(temp_dir.path / "second.html");
  const results_snapshot snapshot(make_collection());

  REQUIRE(write_html_report(html_path, snapshot, true, 3));
  const auto live_html(read_text_file(html_path));
  const std::string marker("<script src=\"");
  const auto source_start(live_html.find(marker));
  REQUIRE(source_start != std::string::npos);
  const auto name_start(source_start + marker.size());
  const auto name_end(live_html.find('"', name_start));
  REQUIRE(name_end != std::string::npos);
  const auto script_name(live_html.substr(name_start, name_end - name_start));
  CHECK(script_name == "report.echarts.min.js");
  const auto script_path(temp_dir.path / script_name);
  REQUIRE(fs::is_regular_file(script_path));
  const auto script(read_text_file(script_path));
  CHECK(live_html.size() < script.size());
  CHECK_FALSE(live_html.contains(script));

  const auto old_time(fs::file_time_type::clock::now() - 24h);
  fs::last_write_time(script_path, old_time);
  const auto saved_time(fs::last_write_time(script_path));
  REQUIRE(write_html_report(html_path, snapshot, true, 3));
  CHECK(fs::last_write_time(script_path) == saved_time);

  const auto second_script_path(temp_dir.path / "second.echarts.min.js");
  REQUIRE(write_html_report(second_html_path, snapshot, true, 3));
  CHECK(read_text_file(second_html_path)
        .contains("<script src=\"second.echarts.min.js\">"));
  REQUIRE(fs::is_regular_file(second_script_path));
  CHECK(fs::last_write_time(script_path) == saved_time);

  REQUIRE(write_html_report(html_path, snapshot));
  const auto final_html(read_text_file(html_path));
  CHECK_FALSE(final_html.contains(marker));
  CHECK(final_html.contains(script));
  CHECK_FALSE(fs::is_regular_file(script_path));
  CHECK(fs::is_regular_file(second_script_path));

  REQUIRE(write_html_report(second_html_path, snapshot));
  CHECK_FALSE(fs::is_regular_file(second_script_path));
}

TEST_CASE("Companion script URLs encode special filename characters")
{
  const auto built(build_echarts_html("Title", "{}", "<div></div>", "",
                                      "a#b%c?d\"e&f.js"));
  CHECK(built.contains("<script src=\"a%23b%25c%3Fd%22e%26f.js\"></script>"));

  const temp_dir_guard temp_dir("wopr_test_encoded_companion");
  const auto html_path(temp_dir.path / "run#1%2& test.html");
  const auto script_path(temp_dir.path / "run#1%2& test.echarts.min.js");
  const results_snapshot snapshot(make_collection());

  REQUIRE(write_html_report(html_path, snapshot, true, 3));
  REQUIRE(fs::is_regular_file(script_path));
  const auto live_html(read_text_file(html_path));
  CHECK(live_html.contains(
    "<script src=\"run%231%252%26%20test.echarts.min.js\"></script>"));

  REQUIRE(write_html_report(html_path, snapshot));
  CHECK_FALSE(fs::is_regular_file(script_path));
  CHECK_FALSE(read_text_file(html_path).contains("<script src="));
}

TEST_CASE("Summary HTML export loads XML files and writes report")
{
  const temp_dir_guard temp_dir("wopr_test_html_summary");
  const auto xml_path(temp_dir.path / "iris.summary.xml");
  const auto html_path(temp_dir.path / "summary.html");

  const std::string xml(ultra::crc32::embed_xml_signature(
    "<ultra><summary><runs>4</runs><elapsed_time>2000</elapsed_time>"
    "<success_rate>0.75</success_rate>"
    "<distributions><fitness><mean>0.8</mean>"
    "<standard_deviation>0.05</standard_deviation></fitness></distributions>"
    "<best><fitness>0.99</fitness><code>x + 1</code></best>"
    "<elite><run id=\"2\"><fitness>0.99</fitness>"
    "<accuracy>0.95</accuracy></run></elite>"
    "</summary><checksum>00000000</checksum></ultra>"));
  std::ofstream(xml_path, std::ios::binary) << xml;

  auto collection(
    rs::setup_collection(temp_dir.path, {}, rs::exec_mode::summary));
  REQUIRE(collection.size() == 1);

  CHECK(rs::summary::start({
    .collection = std::move(collection),
    .html_output = html_path
  }));
  REQUIRE(fs::is_regular_file(html_path));

  const auto written(read_text_file(html_path));
  CHECK(written.contains("\"name\":\"iris\""));
  CHECK(written.contains("\"current\":{\"runs\":4"));
  CHECK(written.contains("\"best_prg\":\"x + 1\""));

  CHECK_FALSE(rs::summary::start({
    .collection = rs::setup_collection(temp_dir.path, {}, rs::exec_mode::summary),
    .html_output = temp_dir.path / "missing_subdir" / "summary.html"
  }));
}

TEST_CASE("Headless run exports HTML report after completion")
{
  const temp_dir_guard temp_dir("wopr_test_html_run");
  const auto csv_path(temp_dir.path / "tiny.csv");
  const auto html_path(temp_dir.path / "run_report.html");
  std::ofstream(csv_path) << "x,y\n1,2\n2,4\n3,6\n4,8\n5,10\n6,12\n";

  rs::settings settings;
  settings.generations = 1;
  settings.runs = 1;
  auto collection(
    rs::setup_collection(csv_path, {}, rs::exec_mode::run, settings));
  REQUIRE(collection.size() == 1);

  CHECK(rs::run::start({
    .collection = std::move(collection),
    .threads = 1,
    .html_output = html_path,
    .interval = 0
  }));
  REQUIRE(fs::is_regular_file(html_path));

  const auto written(read_text_file(html_path));
  CHECK(written.contains("\"name\":\"tiny\""));
  CHECK(written.contains("\"status\":\"Finished\""));
  CHECK(written.contains("\"current\":{\"runs\":1"));
  CHECK_FALSE(fs::is_regular_file(temp_dir.path / "run_report.echarts.min.js"));
}

TEST_CASE("Failed run does not export stale summary from previous invocation")
{
  const temp_dir_guard temp_dir("wopr_test_stale_summary");
  const auto bad_csv(temp_dir.path / "broken.csv");
  const auto bad_xml(temp_dir.path / "broken.summary.xml");
  const auto good_csv(temp_dir.path / "healthy.csv");
  const auto html_path(temp_dir.path / "report.html");

  std::ofstream(bad_csv) << "";  // triggers dataframe failure before summary
  std::ofstream(good_csv) << "x,y\n1,2\n2,4\n3,6\n4,8\n5,10\n6,12\n";

  const std::string stale_xml(ultra::crc32::embed_xml_signature(
    "<ultra><summary><runs>4</runs><elapsed_time>4000</elapsed_time>"
    "<success_rate>1.0</success_rate>"
    "<distributions><fitness><mean>0.9</mean>"
    "<standard_deviation>0.01</standard_deviation></fitness></distributions>"
    "<best><fitness>1.0</fitness><code>old_program</code></best>"
    "</summary><checksum>00000000</checksum></ultra>"));
  std::ofstream(bad_xml, std::ios::binary) << stale_xml;

  rs::settings settings;
  settings.generations = 1;
  settings.runs = 1;
  auto collection(
    rs::setup_collection(temp_dir.path, {}, rs::exec_mode::run, settings));
  REQUIRE(collection.size() == 2);

  CHECK_FALSE(rs::run::start({
    .collection = std::move(collection),
    .threads = 1,
    .html_output = html_path,
    .interval = 0
  }));
  REQUIRE(fs::is_regular_file(html_path));

  const auto written(read_text_file(html_path));
  CHECK(written.contains("\"name\":\"broken\""));
  CHECK(written.contains("\"name\":\"healthy\""));
  CHECK(written.contains("\"status\":\"Failed\""));
  CHECK(written.contains("\"status\":\"Finished\""));
  CHECK(written.contains("\"current\":{\"runs\":0"));
  CHECK(written.contains("\"current\":{\"runs\":1"));
  CHECK_FALSE(written.contains("old_program"));
}

TEST_CASE("Read-only existing summary does not leak stale data into run report")
{
  temp_dir_guard temp_dir("wopr_test_readonly_summary");
  const auto csv_path(temp_dir.path / "tiny.csv");
  const auto xml_path(temp_dir.path / "tiny.summary.xml");
  const auto html_path(temp_dir.path / "report.html");
  temp_dir.protected_files.push_back(xml_path);

  std::ofstream(csv_path) << "x,y\n1,2\n2,4\n3,6\n4,8\n5,10\n6,12\n";

  const std::string stale_xml(ultra::crc32::embed_xml_signature(
    "<ultra><summary><runs>4</runs><elapsed_time>4000</elapsed_time>"
    "<success_rate>1.0</success_rate>"
    "<distributions><fitness><mean>0.9</mean>"
    "<standard_deviation>0.01</standard_deviation></fitness></distributions>"
    "<best><fitness>1.0</fitness><code>old_program</code></best>"
    "</summary><checksum>00000000</checksum></ultra>"));
  std::ofstream(xml_path, std::ios::binary) << stale_xml;
  fs::permissions(xml_path, fs::perms::owner_read, fs::perm_options::replace);

  rs::settings settings;
  settings.generations = 1;
  settings.runs = 1;
  auto collection(
    rs::setup_collection(csv_path, {}, rs::exec_mode::run, settings));
  REQUIRE(collection.size() == 1);

  CHECK(rs::run::start({
    .collection = std::move(collection),
    .threads = 1,
    .html_output = html_path,
    .interval = 0
  }));
  REQUIRE(fs::is_regular_file(html_path));

  const auto written(read_text_file(html_path));
  CHECK(written.contains("\"name\":\"tiny\""));
  CHECK(written.contains("\"status\":\"Finished\""));
  CHECK(written.contains("\"current\":{\"runs\":1"));
  CHECK_FALSE(written.contains("old_program"));
}

TEST_CASE("Live run updates and finalizes HTML report")
{
  const temp_dir_guard temp_dir("wopr_test_live_html_run");
  const auto csv_path(temp_dir.path / "tiny.csv");
  const auto html_path(temp_dir.path / "live_run.html");
  std::ofstream(csv_path) << "x,y\n1,2\n2,4\n3,6\n4,8\n5,10\n6,12\n";

  rs::settings settings;
  settings.generations = 1;
  settings.runs = 1;
  auto collection(
    rs::setup_collection(csv_path, {}, rs::exec_mode::run, settings));
  REQUIRE(collection.size() == 1);

  CHECK(rs::run::start({
    .collection = std::move(collection),
    .threads = 1,
    .html_output = html_path,
    .interval = 1
  }));
  REQUIRE(fs::is_regular_file(html_path));

  const auto written(read_text_file(html_path));
  CHECK(written.contains("\"name\":\"tiny\""));
  CHECK(written.contains("\"live\":false"));
  CHECK(written.contains("\"status\":\"Finished\""));
  CHECK(written.contains("\"current\":{\"runs\":1"));
  CHECK_FALSE(fs::is_regular_file(temp_dir.path / "live_run.echarts.min.js"));
}

TEST_CASE("CLI defaults place HTML reports in each command directory and honor --html")
{
  const temp_dir_guard temp_dir("wopr_test_cli_html_defaults");
  const auto exp_dir(temp_dir.path / "exp");
  const auto log_dir(temp_dir.path / "logs");
  const auto ref_dir(temp_dir.path / "ref");
  fs::create_directories(exp_dir);
  fs::create_directories(log_dir);
  fs::create_directories(ref_dir);

  const auto csv_path(exp_dir / "tiny.csv");
  const auto xml_path(exp_dir / "tiny.summary.xml");
  const auto dyn_path(log_dir / "dynamic.txt");
  const auto custom_log(temp_dir.path / "custom_dynamic.txt");
  std::ofstream(csv_path) << "x,y\n1,2\n2,4\n";
  std::ofstream(xml_path) << "<ultra></ultra>\n";
  std::ofstream(dyn_path) << "";
  std::ofstream(custom_log) << "";

  const auto parse([](std::initializer_list<const char *> args)
  {
    std::vector<std::string> storage;
    std::vector<char *> argv;
    for (const char *arg : args)
      storage.emplace_back(arg);
    for (auto &s : storage)
      argv.push_back(s.data());
    return parse_args(static_cast<int>(argv.size()), argv.data());
  });

  SUBCASE("run defaults to output directory and allows override or disabling")
  {
    const auto dir_cmd(parse({"wopr", "run", exp_dir.string().c_str()}));
    REQUIRE(dir_cmd);
    REQUIRE(std::holds_alternative<rs::run::options>(*dir_cmd));
    CHECK(std::get<rs::run::options>(*dir_cmd).html_output
          == (exp_dir / "run.html").lexically_normal());

    const auto file_cmd(parse({"wopr", "run", csv_path.string().c_str()}));
    REQUIRE(file_cmd);
    CHECK(std::get<rs::run::options>(*file_cmd).html_output
          == (exp_dir / "run.html").lexically_normal());

    const auto override_cmd(
      parse({"wopr", "run", exp_dir.string().c_str(), "--html", "custom.html"}));
    REQUIRE(override_cmd);
    CHECK(std::get<rs::run::options>(*override_cmd).html_output == "custom.html");

    const auto disabled_cmd(
      parse({"wopr", "run", exp_dir.string().c_str(), "--html", ""}));
    REQUIRE(disabled_cmd);
    CHECK(std::get<rs::run::options>(*disabled_cmd).html_output.empty());
  }

  SUBCASE("monitor defaults beside monitored logs and allows override")
  {
    const auto dir_cmd(parse({"wopr", "monitor", log_dir.string().c_str()}));
    REQUIRE(dir_cmd);
    REQUIRE(std::holds_alternative<monitor::options>(*dir_cmd));
    CHECK(std::get<monitor::options>(*dir_cmd).html_output
          == (log_dir / "monitor.html").lexically_normal());

    const auto custom_cmd(parse({
      "wopr", "monitor", log_dir.string().c_str(),
      "--dynamic", custom_log.string().c_str()
    }));
    REQUIRE(custom_cmd);
    CHECK(std::get<monitor::options>(*custom_cmd).html_output
          == (temp_dir.path / "monitor.html").lexically_normal());

    const auto override_cmd(parse({
      "wopr", "monitor", log_dir.string().c_str(), "--html", "custom_mon.html"
    }));
    REQUIRE(override_cmd);
    CHECK(std::get<monitor::options>(*override_cmd).html_output
          == "custom_mon.html");
  }

  SUBCASE("summary defaults to first input directory and allows override")
  {
    const auto dir_cmd(parse({
      "wopr", "summary", exp_dir.string().c_str(), ref_dir.string().c_str()
    }));
    REQUIRE(dir_cmd);
    REQUIRE(std::holds_alternative<rs::summary::options>(*dir_cmd));
    CHECK(std::get<rs::summary::options>(*dir_cmd).html_output
          == (exp_dir / "summary.html").lexically_normal());

    const auto override_cmd(parse({
      "wopr", "summary", exp_dir.string().c_str(), "--html", "custom_sum.html"
    }));
    REQUIRE(override_cmd);
    CHECK(std::get<rs::summary::options>(*override_cmd).html_output
          == "custom_sum.html");
  }
}

}  // TEST_SUITE("WOPR RESULTS")
