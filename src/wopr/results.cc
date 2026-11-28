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

#include "html_report.h"

#include "kernel/search_log.h"
#include "utility/misc.h"

#include "tinyxml2/tinyxml2.h"

#include <algorithm>
#include <cassert>
#include <expected>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>

namespace fs = std::filesystem;

namespace ultra::wopr
{

using namespace std::chrono_literals;

fs::path build_path(fs::path base_dir, fs::path f)
{
  if (f.empty())
    return {};

  if (f.is_absolute())
    return f.lexically_normal();

  if (base_dir.empty())
    base_dir = "./";

  return base_dir / f.lexically_normal();
}

bool summary_data::check(const fs::path &xml_fn, tinyxml2::XMLDocument &doc)
{
  std::ifstream in(xml_fn, std::ios::binary);
  if (!in)
    return false;

  std::ostringstream ss;
  if (!(ss << in.rdbuf()))
    return false;

  const std::string xml_content(ss.str());

  return ultra::crc32::verify_xml_signature(xml_content)
         && doc.Parse(xml_content.data(), xml_content.size())
            == tinyxml2::XML_SUCCESS
         && doc.FirstChild();
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
    if (const auto *e = h_best.FirstChildElement("code").ToElement())
      best_prg = e->GetText();
  }

  {
    const auto h_elite(h_summary.FirstChildElement("elite"));

    auto auto_id(std::numeric_limits<decltype(runs)>::max() / 2);

    if (const auto *elite_el = h_elite.ToElement())
    {
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

namespace rs
{

std::expected<ultra::model_measurements<double>, std::string>
extract_threshold(std::string_view txt)
{
  if (txt.empty())
    return {};

  ultra::model_measurements<double> threshold;

  const bool accuracy(txt.ends_with('%'));
  if (accuracy)
    txt.remove_suffix(1);

  try
  {
    std::size_t consumed;
    const std::string s(txt);
    const double val(std::stod(s, &consumed));

    if (consumed != s.size())
      return std::unexpected("Invalid threshold.");

    if (accuracy)
    {
      threshold.accuracy = val / 100.0;

      if (threshold.accuracy < 0.0 || threshold.accuracy > 1.0)
        return std::unexpected("Wrong range for accuracy.");
    }
    else
      threshold.fitness = val;
  }
  catch (...)
  {
    return std::unexpected("Cannot convert threshold.");
  }

  return threshold;
}

settings read_settings(const fs::path &test_fn, const settings &defaults)
{
  assert(ultra::iequals(test_fn.extension().string(), ".csv"));

  const auto settings_fn(fs::path(test_fn).replace_extension(".xml"));
  if (!fs::exists(settings_fn))
    return defaults;

  tinyxml2::XMLDocument doc;
  if (const auto result(doc.LoadFile(settings_fn.string().c_str()));
      result != tinyxml2::XML_SUCCESS)
  {
    std::cerr << "Cannot open settings for " << test_fn << ".\n";
    return defaults;
  }

  settings ret(defaults);

  tinyxml2::XMLConstHandle handle(&doc);
  const auto h_ultra(handle.FirstChildElement("ultra"));

  if (const auto *e =
      h_ultra.FirstChildElement("common_random_numbers").ToElement())
    ret.common_random_numbers = e->BoolText(ret.common_random_numbers);

  const auto h_search(h_ultra.FirstChildElement("search"));
  if (const auto *e = h_search.FirstChildElement("generations").ToElement())
    ret.generations = e->UnsignedText(ret.generations);

  if (const auto *e = h_search.FirstChildElement("runs").ToElement())
    ret.runs = e->UnsignedText(ret.runs);

  if (const auto *e = h_search.FirstChildElement("threshold").ToElement())
    if (const char *text = e->GetText())
    {
      if (const auto threshold(extract_threshold(text)); threshold)
        ret.threshold = threshold.value();
      else
        std::cerr << threshold.error() << '\n';
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

summary_data load_reference(const fs::path &ref_dir, const fs::path &base)
{
  if (ref_dir.empty())
  {
    std::cout << "\n  no reference\n";
    return {};
  }

  if (const auto ref(ref_dir / ultra::summary_from_basename(base.filename()));
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

  return {};
}

void show_settings(std::string_view name, const settings &ts)
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

  if (ts.common_random_numbers)
    std::cout << "\n  Common random numbers: yes";

  std::cout << '\n';
}

void insert_dataset(collection_t &collection, const fs::path &basename,
                    const fs::path &summary, const settings &conf,
                    const fs::path &ref_dir, bool has_settings)
{
  std::cout << "Basename: " << basename
            << "\n  first summary: " << summary;

  collection.push_back({
    .name = basename.stem().string(),
    .dataset = basename,
    .xml_summary = summary,
    .conf = conf,
    .current = {},
    .reference = load_reference(ref_dir, basename)
  });

  if (has_settings)
    show_settings(collection.back().name, collection.back().conf);
}

collection_t setup_collection(fs::path in1, fs::path in2, exec_mode m,
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
    const auto ext(path.extension().string());

    if (m == exec_mode::run && ultra::iequals(ext, ".csv"))
    {
      insert_dataset(ret, path, summary_from_basename(path),
                     read_settings(path, defaults), in2, true);
      return true;
    }

    if (m == exec_mode::summary && ultra::iequals(ext, ".xml")
        && path.stem().string().ends_with(".summary"))
    {
      insert_dataset(ret, basename_from_summary(path), path, {}, in2, false);
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

      if (const auto ext(p.extension().string());
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
    std::cout << ' ' << ds.name;
  std::cout << '\n';

  return ret;
}

ultra::search_log make_search_log(const fs::path &dataset,
                                  const fs::path &summary)
{
  ultra::search_log sl;
  const auto base_dir(dataset.parent_path());

  sl.dynamic_file_path = build_path(
    base_dir, ultra::dynamic_from_basename(dataset.string()));
  sl.layers_file_path = build_path(
    base_dir, ultra::layers_from_basename(dataset.string()));
  sl.population_file_path = build_path(
    base_dir, ultra::population_from_basename(dataset.string()));
  sl.summary_file_path = summary.empty() ? ultra::summary_from_basename(dataset)
                                         : summary;

  return sl;
}

void dataset_activity::set(task_state state, std::string error)
{
  std::lock_guard guard(mutex_);
  status_ = {state, std::move(error)};
}

task_status dataset_activity::status() const
{
  std::lock_guard guard(mutex_);
  return status_;
}

}  // namespace rs

namespace
{

[[nodiscard]] side_snapshot make_side_snapshot(const summary_data &summary)
{
  constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

  side_snapshot out;
  out.runs = summary.runs;
  if (!summary.empty())
  {
    out.success_rate = summary.success_rate * 100.0;
    out.time_per_run =
      std::chrono::duration<double>(summary.elapsed_time).count()
      / summary.runs;
  }

  out.fit_best = summary.best_fit.size() ? summary.best_fit[0] : qnan;
  out.fit_mean = summary.fit_mean.size() ? summary.fit_mean[0] : qnan;
  out.fit_std_dev = summary.fit_std_dev.size() ? summary.fit_std_dev[0] : qnan;
  out.best_prg = summary.best_prg;

  out.elite.reserve(summary.elite.size());
  for (const auto &[id, measurements] : summary.elite)
  {
    out.elite.push_back({
      .id = id,
      .fit = measurements.fitness ? (*measurements.fitness)[0] : qnan,
      .acc = measurements.accuracy ? *measurements.accuracy : qnan
    });
  }

  return out;
}

void load_current_summaries(rs::collection_t &collection)
{
  for (auto &test : collection)
    if (test.current.empty() && !test.xml_summary.empty())
      if (tinyxml2::XMLDocument summary;
          summary_data::check(test.xml_summary, summary))
        test.current = summary_data(summary);
}

}  // namespace

std::optional<double> estimated_time_remaining(const dataset_snapshot &ds)
{
  if (ds.current.runs >= ds.capacity_runs)
    return 0.0;
  if (!ds.current.runs)
    return std::nullopt;

  return ds.current.time_per_run * (ds.capacity_runs - ds.current.runs);
}

std::string remaining_time_label(const dataset_snapshot &ds)
{
  const auto estimate(estimated_time_remaining(ds));
  if (!estimate)
    return "Pending";
  if (ds.current.runs >= ds.capacity_runs)
    return "Complete";

  using namespace std::chrono;
  const auto total(ceil<seconds>(duration<double>(*estimate)));
  const auto h(duration_cast<hours>(total));
  const auto m(duration_cast<minutes>(total - h));
  const auto s(total - h - m);
  if (h.count())
    return std::format("~{}h {}m {}s", h.count(), m.count(), s.count());
  if (m.count())
    return std::format("~{}m {}s", m.count(), s.count());
  return std::format("~{}s", s.count());
}

results_snapshot::results_snapshot(const rs::collection_t &collection,
                                   rs::exec_mode mode)
  : references_available(std::ranges::any_of(
      collection, [](const auto &entry) { return !entry.reference.empty(); }))
{
  datasets.reserve(collection.size());

  for (const auto &test : collection)
  {
    const bool has_threshold(mode == rs::exec_mode::summary
                             || !test.conf.threshold.empty());
    show_success_rate |= has_threshold;

    dataset_snapshot ds{
      .name = test.name,
      .capacity_runs = test.conf.runs,
      .has_threshold = has_threshold,
      .status = test.activity ? std::optional(test.activity->status())
                              : std::nullopt,
      .current = make_side_snapshot(test.current),
      .reference = make_side_snapshot(test.reference)
    };
    ds.remaining_label = remaining_time_label(ds);

    datasets.push_back(std::move(ds));
  }
}

bool rs::summary::start(options options)
{
  if (options.html_output.empty())
  {
    std::cerr << "Missing HTML output path for `summary`.\n";
    return false;
  }

  load_current_summaries(options.collection);
  const results_snapshot snapshot(options.collection);
  return write_html_report(options.html_output, snapshot);
}

}  // namespace ultra::wopr
