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

#include "imgui_app.h"

#include "kernel/exceptions.h"
#include "kernel/search_log.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/search.h"
#include "utility/log.h"
#include "utility/timer.h"
#include "utility/ts_queue.h"

#include "argh/argh.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <string_view>

using namespace std::chrono_literals;

using crossover_types_map = std::map<int, unsigned>;


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


/*********************************************************************
 * Dynamic file - related data structures
 ********************************************************************/
class dynamic_data
{
public:
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

  crossover_types_map crossover_types {};

  std::string best_prg {};

private:
  static bool parse_python_dict(std::istringstream &, crossover_types_map &);
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
        || !parse_python_dict(ss, crossover_types)
        || !(ss >> std::ws)
        || !std::getline(ss, best_prg))
      throw ultra::exception::data_format("Cannot parse dynamic file line");
  }
}

bool dynamic_data::parse_python_dict(std::istringstream &ss,
                                     crossover_types_map &m)
{
  char c;
  int key;
  unsigned value;

  m = {};

  ss >> std::skipws >> c;  // '{'
  if (!ss || c != '{')
    return false;

  ss >> std::skipws;
  if (ss.peek() == '}')  // empty map
  {
    ss.get();
    return true;
  }

  while (true)
  {
    if (!(ss >> key))
      return false;

    ss >> std::skipws >> c;
    if (!ss || c != ':')
      return false;

    if (!(ss >> value))
      return false;

    m[key] = value;

    if (!(ss >> std::skipws >> c))  // ',' or '}'
      return false;

    if (c == '}')
      return true;
    if (c != ',')
      return false;
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

  std::map<crossover_types_map::key_type,
           std::vector<double>> crossover_types {};

  std::vector<std::string> best_prg {};

  // This keeps track of the index of selected program inside the GUI.
  int best_idx {};

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

    for (const auto &[key, val] : dd.crossover_types)
      crossover_types[key].push_back(val);

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

    fit.push_back(fit_val[0]);
    obs.push_back(obs_val);
  }
}

ultra::ts_queue<population_line> population_queue;

struct population_sequence
{
  std::vector<double> fit {};
  std::vector<double> obs {};

  std::vector<double> xs {};
  std::vector<double> fit_entropy {};

  unsigned generation {0};

  [[nodiscard]] bool empty() const noexcept { return fit.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return fit.size(); }

  void update(population_line &pl)
  {
    generation = pl.generation;

    fit = std::move(pl.fit);
    obs = std::move(pl.obs);

    xs.push_back(fit_entropy.size());
    fit_entropy.push_back(calculate_entropy());
  }

  // Returns the entropy of the distribution.
  //
  // \f$H(X)=-\sum_{i=1}^n p(x_i) \dot log_b(p(x_i))\f$
  //
  // Offline algorithm: https://en.wikipedia.org/wiki/Online_algorithm.
  [[nodiscard]] double calculate_entropy() const
  {
    constexpr double c(1.0 / std::numbers::ln2_v<double>);

    const auto pop_size(std::accumulate(obs.begin(), obs.end(), 0.0));

    if (!pop_size)
      return 0;

    double h(0.0);
    for (auto x : obs)
      if (x > 0.0)
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

  [[nodiscard]] bool empty() const noexcept { return !runs; }

  unsigned runs {0};
  std::chrono::milliseconds elapsed_time {0};
  double success_rate {0.0};

  ultra::fitnd fit_mean {};
  ultra::fitnd fit_std_dev {};

  ultra::fitnd best_fit {-std::numeric_limits<double>::infinity()};
  double best_accuracy {-std::numeric_limits<double>::infinity()};

  unsigned    best_run {};
  std::string best_prg {};

  std::set<unsigned> good_runs {};
  std::vector<
    std::pair<unsigned, ultra::model_measurements<ultra::fitnd>>> elite {};
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
    if (const auto *e = h_best.FirstChildElement("accuracy").ToElement())
      best_accuracy = e->DoubleText(0.0);
    if (const auto *e = h_best.FirstChildElement("run").ToElement())
      best_run = e->UnsignedText(0);
    if (const auto *e = h_best.FirstChildElement("code").ToElement())
      best_prg = e->GetText();
  }

  {
    const auto h_solutions(h_summary.FirstChildElement("solutions"));

    for (const auto *e(h_solutions.FirstChildElement().ToElement()); e;
         e = e->NextSiblingElement())
      if (unsigned run; e->QueryUnsignedText(&run) == tinyxml2::XML_SUCCESS)
        good_runs.insert(run);
  }

  {
    const auto h_elite(h_summary.FirstChildElement("elite"));

    auto auto_id(std::numeric_limits<decltype(runs)>::max() / 2);

    if (const auto *elite_el = h_elite.ToElement())
    {
      // unsigned percentile = elite_el->UnsignedAttribute("percentile", 0);

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


/*********************************************************************
 * Misc data.
 ********************************************************************/
namespace monitor  // monitoring related variables
{
ultra::search_log slog {};
int window {0};

void start(const imgui_app::program::settings &);
[[nodiscard]] bool setup_cmd(argh::parser &);
}

namespace rs  // running, testing and comparison related variables
{

enum class exec_mode {run, summary};

struct settings
{
  static unsigned default_generations;
  static unsigned default_runs;
  static ultra::model_measurements<double> default_threshold;

  ultra::src::dataframe::params params {};

  unsigned generations {default_generations};
  unsigned runs {default_runs};
  ultra::model_measurements<double> threshold {default_threshold};
};

unsigned settings::default_generations {100};
unsigned settings::default_runs {1};
ultra::model_measurements<double> settings::default_threshold {};

struct data
{
  data(const std::filesystem::path &ds,
       const std::filesystem::path &xs,
       const settings &c, const summary_data &r)
    : dataset(ds), xml_summary(xs), conf(c), reference(r)
  {
  }

  const std::filesystem::path dataset;
  const std::filesystem::path xml_summary;

  const settings conf;
  summary_data current {};
  const summary_data reference;
};

// Protects `data.current`. `std::shared_mutex` is used to allow multiple
// concurrent readers, while writes (updates to `data.current`) are exclusive.
// `data.reference` doesn't need a mutex since it's compiled once at program
// startup.
std::shared_mutex current_mutex;

using collection_t = std::vector<std::pair<const std::string, data>>;

// After initialisation, `collection` has a stable size, stable ordering,
// stable keys, `dataset`, `xml_summary`, `conf` and `reference`; only
// `current` mutates.
collection_t collection;

[[nodiscard]] settings read_settings(const std::filesystem::path &);
[[nodiscard]] bool references_available() noexcept;
[[nodiscard]] collection_t setup_collection(std::filesystem::path,
                                            std::filesystem::path, exec_mode);

namespace run
{
bool nogui {false};

[[nodiscard]] bool setup_cmd(argh::parser &);
void start(const imgui_app::program::settings &);
}  // namespace run

namespace summary
{
void get_summaries(std::stop_token);
[[nodiscard]] bool setup_cmd(argh::parser &);
void start(const imgui_app::program::settings &);
}  // namespace summary

}  // namespace rs

// Other variables and functions.
const char *current_str = "Current";
const char *reference_str = "Reference";

bool imgui_demo_panel {false};

using labels_data = std::vector<const char *>;

[[nodiscard]] labels_data make_labels(const rs::collection_t &);
[[nodiscard]] std::vector<double> make_positions(std::size_t);
[[nodiscard]] std::string random_string();
[[nodiscard]] std::vector<const char *> to_cstr_vector(
  const std::vector<std::string> &);


/*********************************************************************
 * Rendering
 ********************************************************************/
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
// - `path`, `conf` and `reference` are immutable;
// - only `current` is synchronised by `current_mutex`.
void render_number_of_runs()
{
  static const std::vector ilabels {current_str, reference_str};

  constexpr double bar_width(0.5), half_width(bar_width / 2.0);

  static const std::size_t size(rs::collection.size());
  if (!size)
    return;

  static bool show_reference_values {rs::references_available()};
  if (rs::references_available())
    ImGui::Checkbox("Reference values##Run##Runs", &show_reference_values);

  const std::size_t group_count(1 + show_reference_values);

  struct cap_data
  {
    std::vector<unsigned> runs;
    unsigned max;
  };

  static const cap_data caps = []
  {
    cap_data out;
    out.max = 0;
    out.runs.reserve(size);

    for (const auto &[_, data] : rs::collection)
    {
      out.runs.push_back(data.conf.runs);
      out.max = std::max(out.max, data.conf.runs);
    }

    return out;
  }();

  std::vector<unsigned> bar_values;
  bar_values.reserve(size * group_count);

  unsigned max_runs(caps.max);

  for (std::shared_lock guard(rs::current_mutex);
       const auto &[_, data] : rs::collection)
  {
    bar_values.push_back(data.current.runs);
    max_runs = std::max(max_runs, data.current.runs);
  }

  if (show_reference_values)
    for (const auto &[_, data] : rs::collection)
    {
      bar_values.push_back(data.reference.runs);
      max_runs = std::max(max_runs, data.reference.runs);
    }

  static const auto labels(make_labels(rs::collection));
  static const auto positions(make_positions(size));

  int flags(ImPlotFlags_NoTitle);
  if (!rs::references_available() || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Runs##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Runs", ImPlotAxisFlags_AutoFit);
  ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), size, labels.data());
  ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, max_runs + 1.0);

  // Bars.
  ImPlot::PlotBarGroups(ilabels.data(), bar_values.data(), group_count, size,
                        bar_width);

  // Capacity lines.
  for (std::size_t i(0); i < size; ++i)
    if (caps.runs[i] > 1)
    {
      const auto di(static_cast<double>(i));
      const auto dcp(static_cast<double>(caps.runs[i]));

      const double xs[] =
      {
        di - half_width, show_reference_values ? di : di + half_width
      };

      const double ys[] = { dcp, dcp };

      ImPlot::SetNextLineStyle(ImVec4(1, 1, 0, 1), 2.5f); // yellow, thick
      ImPlot::PlotLine("##TotalRuns", xs, ys, 2);
    }

  ImPlot::EndPlot();
}

void render_success_rate()
{
  static const std::vector ilabels {current_str, reference_str};

  static const std::size_t size(rs::collection.size());
  if (!size)
    return;

  static bool show_reference_values {rs::references_available()};
  if (rs::references_available())
    ImGui::Checkbox("Reference values##Run##Success rate",
                    &show_reference_values);
  const std::size_t group_count(1 + show_reference_values);

  std::vector<double> sr;
  sr.reserve(size * group_count);

  double best_success_rate(0.0);
  for (std::shared_lock guard(rs::current_mutex);
       const auto &[_, data] : rs::collection)
  {
    best_success_rate = std::max(data.current.success_rate, best_success_rate);
    sr.push_back(data.current.success_rate * 100.0);
  }

  if (show_reference_values)
    for (const auto &[_, data] : rs::collection)
    {
      best_success_rate = std::max(data.reference.success_rate,
                                   best_success_rate);
      sr.push_back(data.reference.success_rate * 100.0);
    }

  static const auto labels(make_labels(rs::collection));
  static const auto positions(make_positions(size));

  int flags(ImPlotFlags_NoTitle);
  if (!rs::references_available() || !show_reference_values)
    flags |= ImPlotFlags_NoLegend;

  if (!ImPlot::BeginPlot("##Success rate##Run", ImVec2(-1, -1), flags))
    return;

  if (show_reference_values)
    ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);

  ImPlot::SetupAxes("Dataset", "Success rate", ImPlotAxisFlags_AutoFit);
  ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), size, labels.data());

  if (best_success_rate > 0.0)
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0,
                            std::min(100.0, 100.0 * best_success_rate + 5.0),
                            ImGuiCond_Always);
  else
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);

  ImPlot::PlotBarGroups(ilabels.data(), sr.data(), group_count, size, 0.5);
  ImPlot::EndPlot();
}

void render_fitness_across_datasets()
{
  static const std::size_t size(rs::collection.size());
  if (!size)
    return;

  static bool show_reference_values {rs::references_available()};
  if (rs::references_available())
    ImGui::Checkbox("Reference values##FAD", &show_reference_values);

  struct fit_data
  {
    fit_data()
    {
      best.reserve(size);
      mean.reserve(size);
      std_dev.reserve(size);
      runs.reserve(size);
    }

    std::vector<double> best;
    std::vector<double> mean;
    std::vector<double> std_dev;
    std::vector<double> runs;
  };

  const auto add_data([](fit_data &fitd, const summary_data &sumd)
  {
    constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

    if (sumd.best_fit.size())
      fitd.best.push_back(sumd.best_fit[0]);
    else
      fitd.best.push_back(qnan);

    if (sumd.fit_mean.size())
      fitd.mean.push_back(sumd.fit_mean[0]);
    else
      fitd.mean.push_back(qnan);

    if (sumd.fit_std_dev.size())
      fitd.std_dev.push_back(sumd.fit_std_dev[0]);
    else
      fitd.std_dev.push_back(qnan);

    fitd.runs.push_back(sumd.runs);
  });

  fit_data current;

  for (std::shared_lock guard(rs::current_mutex);
       const auto &[_, data] : rs::collection)
    add_data(current, data.current);

  static const fit_data reference = [&add_data]
  {
    fit_data out;

    for (const auto &[_, data] : rs::collection)
      add_data(out, data.reference);

    return out;
  }();

  static const auto labels(make_labels(rs::collection));
  static const char title[] = "##FAD";
  static const auto n(static_cast<std::size_t>(std::ceil(std::sqrt(size))));

  if (!ImPlot::BeginSubplots(title, n, n, ImVec2(-1, -1),
                             ImPlotSubplotFlags_NoLegend))
    return;

  for (std::size_t i(0); i < size; ++i)
  {
    bool style_pushed(false);

    if (show_reference_values
        && std::isfinite(current.best[i]) && std::isfinite(reference.best[i])
        && current.best[i] < reference.best[i])
    {
      if (current.mean[i] < reference.mean[i])
      {
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0, 0.0, 0.0, 0.4));
        style_pushed = true;
      }
      else
      {
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(1.0, 0.0, 0.0, 0.2));
        style_pushed = true;
      }
    }

    int flags(0);
    if (!rs::references_available() || !show_reference_values)
      flags |= ImPlotFlags_NoLegend;

    if (ImPlot::BeginPlot(labels[i], ImVec2(-1, -1), flags))
    {
      ImPlot::SetupAxes("Runs", "Fitness", 0, ImPlotAxisFlags_AutoFit);

      const double min_x(reference.runs[i] > 0.0 && show_reference_values
                         ? 0.95 * std::min(current.runs[i], reference.runs[i])
                         : 0.0);
      const double max_x(reference.runs[i] > 0.0 && show_reference_values
                         ? 1.05 * std::max(current.runs[i], reference.runs[i])
                         : 2.0 * std::max(current.runs[i], 1.0));
      //ImPlot::SetupAxesLimits(min_x, max_x, 0.0, 0.0);
      ImPlot::SetupAxisLimits(ImAxis_X1, min_x, max_x, ImGuiCond_Always);

      // Current data.
      {
        const auto xs(current.runs[i]);
        const auto ys(current.mean[i]);
        const auto ys_dev(current.std_dev[i]);
        const auto ys_best(current.best[i]);

        ImPlot::PlotErrorBars(current_str, &xs, &ys, &ys_dev, 1);
        ImPlot::PlotScatter(current_str, &xs, &ys, 1);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up);
        ImPlot::PlotScatter(current_str, &xs, &ys_best, 1);
      }

      // Reference data.
      if (show_reference_values)
      {
        const auto xs(
          reference.runs[i]
          + (ultra::almost_equal(current.runs[i],reference.runs[i])
             ? 0.1 : 0.0));
        const auto ys(reference.mean[i]);
        const auto ys_dev(reference.std_dev[i]);
        const auto ys_best(reference.best[i]);

        ImPlot::PlotErrorBars(reference_str, &xs, &ys, &ys_dev, 1);
        ImPlot::PlotScatter(reference_str, &xs, &ys, 1);
        ImPlot::SetNextMarkerStyle(ImPlotMarker_Up);
        ImPlot::PlotScatter(reference_str, &xs, &ys_best, 1);
      }

      ImPlot::EndPlot();
    }

    if (style_pushed)
      ImPlot::PopStyleColor();
  }

  ImPlot::EndSubplots();
}

void render_elite()
{
  static const std::size_t size(rs::collection.size());
  if (!size)
    return;

  static bool show_reference_values {rs::references_available()};
  if (rs::references_available())
    ImGui::Checkbox("Reference values##ELITE", &show_reference_values);

  struct elite_data
  {
    elite_data() { elite.reserve(size); }

    struct run
    {
      std::vector<unsigned> id;
      std::vector<double> fit;
      std::vector<double> accuracy;
    };

    std::vector<run> elite;
  };

  const auto add_data([](elite_data &e, const summary_data &sumd)
  {
    constexpr double qnan(std::numeric_limits<double>::quiet_NaN());

    elite_data::run d;

    for (std::size_t i(0); i < sumd.elite.size(); ++i)
    {
      d.id.push_back(sumd.elite[i].first);

      if (sumd.elite[i].second.fitness)
        d.fit.push_back((*sumd.elite[i].second.fitness)[0]);
      else
        d.fit.push_back(qnan);

      if (sumd.elite[i].second.accuracy)
        d.accuracy.push_back(*sumd.elite[i].second.accuracy);
      else
        d.accuracy.push_back(qnan);
    }

    e.elite.push_back(d);
  });

  elite_data current;

  for (std::shared_lock guard(rs::current_mutex);
       const auto &[_, data] : rs::collection)
    add_data(current, data.current);

  static const elite_data reference = [&add_data]
  {
    elite_data out;

    for (const auto &[_, data] : rs::collection)
      add_data(out, data.reference);

    return out;
  }();

  static const auto labels(make_labels(rs::collection));
  static const char title[] = "##ELITE";
  static const auto n(static_cast<std::size_t>(std::ceil(std::sqrt(size))));

  if (!ImPlot::BeginSubplots(title, n, n, ImVec2(-1, -1),
                             ImPlotSubplotFlags_NoLegend))
    return;

  for (std::size_t i(0); i < size; ++i)
  {
    int flags(0);
    if (!rs::references_available() || !show_reference_values)
      flags |= ImPlotFlags_NoLegend;

    if (ImPlot::BeginPlot(labels[i], ImVec2(-1, -1), flags))
    {
      ImPlot::SetupAxes("Rank", "Fitness", 0, ImPlotAxisFlags_AutoFit);

      const double max_x(std::max(current.elite[i].id.size(),
                                  reference.elite[i].id.size()));

      ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, max_x + 0.5, ImGuiCond_Always);

      // Current data.
      {
        const auto ys(current.elite[i].fit);

        ImPlot::PlotStems((std::string(current_str) + "##ELITE"
                           + labels[i]).c_str(),
                          ys.data(), ys.size());
      }

      // Reference data.
      if (show_reference_values)
      {
        const auto ys(reference.elite[i].fit);

        ImPlot::PlotStems((std::string(reference_str) + "##ELITE"
                           + labels[i]).c_str(),
                          ys.data(), ys.size(), 0, 1, 0.1);
      }

      ImPlot::EndPlot();
    }
  }

  ImPlot::EndSubplots();
}

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

// Main window for `run` and `summary` (both single/double dir) commands.
void render_rs(const imgui_app::program &prg, bool *p_open)
{
  const auto fa(prg.free_area());
  ImGui::SetNextWindowPos(ImVec2(fa.x, fa.y));
  ImGui::SetNextWindowSize(ImVec2(fa.w, fa.h));

  static bool show_runs_check(true);
  static bool show_success_rate_check(true);
  static bool show_fitness_across_datasets_check(true);
  static bool show_elite_check(true);

  static bool mxz_runs(false);
  static bool mxz_success_rate(false);
  static bool mxz_fitness_across_datasets(false);
  static bool mxz_elite(false);

  if (ImGui::Begin("Run##Window", p_open))
  {
    ImGui::Checkbox("runs", &show_runs_check);
    ImGui::SameLine();
    ImGui::Checkbox("success rate", &show_success_rate_check);
    ImGui::SameLine();
    ImGui::Checkbox("fitness across datasets",
                    &show_fitness_across_datasets_check);
    ImGui::SameLine();
    ImGui::Checkbox("elite", &show_elite_check);
    ImGui::SameLine(ImGui::GetWindowWidth() - 128);
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 0.3f),
                       "%s", random_string().c_str());
    ImGui::Separator();

    const bool show_runs(
      show_runs_check
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_fitness_across_datasets && show_fitness_across_datasets_check)
      && !(mxz_elite && show_elite_check));
    const bool show_success_rate(
      show_success_rate_check
      && !(mxz_runs && show_runs_check)
      && !(mxz_fitness_across_datasets && show_fitness_across_datasets_check)
      && !(mxz_elite && show_elite_check));
    const bool show_fitness_across_datasets(
      show_fitness_across_datasets_check
      && !(mxz_runs && show_runs_check)
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_elite && show_elite_check));
    const bool show_elite(
      show_elite_check
      && !(mxz_runs && show_runs_check)
      && !(mxz_success_rate && show_success_rate_check)
      && !(mxz_fitness_across_datasets && show_fitness_across_datasets_check));

    const int available_width(ImGui::GetContentRegionAvail().x - 4);
    const int available_height(ImGui::GetContentRegionAvail().y - 4);

    const int w1(show_runs && show_success_rate ? available_width/2
                                                : available_width);
    const int h1(show_fitness_across_datasets || show_elite
                 ? available_height/2 : available_height);
    if (show_runs)
    {
      const auto w(mxz_runs ? available_width : w1);
      const auto h(mxz_runs ? available_height : h1);

      ImGui::BeginChild("Runs##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("RUNS");
      ImGui::SameLine();
      if (const char *bs(mxz_runs ? "Minimise##Runs" : "Maximise##Runs");
          ImGui::Button(bs))
        mxz_runs = !mxz_runs;

      render_number_of_runs();
      ImGui::EndChild();
    }

    if (show_success_rate)
    {
      if (show_runs)
        ImGui::SameLine();

      const auto w(mxz_success_rate ? available_width : w1);
      const auto h(mxz_success_rate ? available_height : h1);

      ImGui::BeginChild("Success rate##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("SUCCESS RATE");
      ImGui::SameLine();
      if (const char *bs(mxz_success_rate ? "Minimise##SR" : "Maximise##SR");
          ImGui::Button(bs))
        mxz_success_rate = !mxz_success_rate;

      render_success_rate();
      ImGui::EndChild();
    }

    if (show_fitness_across_datasets)
    {
      const auto w(mxz_fitness_across_datasets ? available_width : w1);
      const auto h(mxz_fitness_across_datasets ? available_height : h1);

      ImGui::BeginChild("FADs##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("FITNESS ACROSS DATASETS");
      ImGui::SameLine();
      if (const char *bs(mxz_fitness_across_datasets
                         ? "Minimise##FitnessAcrossDatasets"
                         : "Maximise##FitnessAcrossDatasets");
          ImGui::Button(bs))
        mxz_fitness_across_datasets = !mxz_fitness_across_datasets;

      render_fitness_across_datasets();
      ImGui::EndChild();
    }

    if (show_elite)
    {
      if (show_fitness_across_datasets)
        ImGui::SameLine();

      const auto w(mxz_fitness_across_datasets ? available_width : w1);
      const auto h(mxz_fitness_across_datasets ? available_height : h1);

      ImGui::BeginChild("ELITEs##ChildWindow", ImVec2(w, h),
                        ImGuiChildFlags_Borders);
      ImGui::AlignTextToFramePadding();
      ImGui::Text("ELITE RUNS");
      ImGui::SameLine();
      if (const char *bs(mxz_elite ? "Minimise##ELITE" : "Maximise##ELITE");
          ImGui::Button(bs))
        mxz_elite = !mxz_elite;

      render_elite();
      ImGui::EndChild();
    }
  }

  // `ImGui::End` is special and must be called even if `Begin` returns false.
  ImGui::End();
}


/*********************************************************************
 * Misc
 ********************************************************************/
std::vector<double> make_positions(std::size_t n)
{
  assert(n);

  std::vector<double> positions(n);
  std::iota(positions.begin(), positions.end(), 0.0);

  return positions;
}

std::vector<const char *> to_cstr_vector(const std::vector<std::string> &v)
{
  std::vector<const char *> out(v.size());

  std::ranges::transform(v, out.begin(),
                         [](const auto &str) noexcept { return str.data(); });

  return out;
}

labels_data make_labels(const rs::collection_t &c)
{
  labels_data out;

  out.reserve(c.size());

  for (const auto &[name, _] : c)
    out.push_back(name.c_str());

  return out;
}

std::string random_string()
{
  constexpr std::size_t length(10);

  static std::string_view charset =
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

class read_log_file
{
public:
  explicit read_log_file(const std::filesystem::path &);

  std::optional<std::string> get_line();

private:
  std::ifstream file_;
  std::streampos position_ {0};
};

read_log_file::read_log_file(const std::filesystem::path &filename)
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

//
// Asynchronously reads all specified log files into queues for subsequent
// processing.
//
void get_logs(std::stop_token stoken)
{
  assert(!monitor::slog.dynamic_file_path.empty()
         || !monitor::slog.layers_file_path.empty()
         || !monitor::slog.population_file_path.empty());

  read_log_file dynamic_log(monitor::slog.dynamic_file_path);
  read_log_file population_log(monitor::slog.population_file_path);
  read_log_file layers_log(monitor::slog.layers_file_path);

  ultra::timer last_read;

  while (!stoken.stop_requested())
  {
    if (auto line(dynamic_log.get_line()); line)
    {
      do
      {
        if (stoken.stop_requested()) break;

        dynamic_queue.push(dynamic_data(*line));
        line = dynamic_log.get_line();
      } while (line);

      last_read.restart();
    }

    if (auto line(population_log.get_line()); line)
    {
      do
      {
        if (stoken.stop_requested()) break;

        population_queue.push(population_line(*line));
        line = population_log.get_line();
      } while (line);

      last_read.restart();
    }

    if (auto line(layers_log.get_line()); line)
    {
      do
      {
        if (stoken.stop_requested()) break;

        layers_queue.push(layers_line(*line));
        line = layers_log.get_line();
      } while (line);

      last_read.restart();
    }

    const auto elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(
                         last_read.elapsed()));
    std::this_thread::sleep_for(std::clamp(elapsed, 100ms, 3000ms));
  }
}

// Asynchronously reads all available summary files into queues for subsequent
// processing.
void rs::summary::get_summaries(std::stop_token stoken)
{
  assert(!collection.empty());

  while (!stoken.stop_requested())
  {
    for (auto &test : collection)
    {
      const auto xml_fn(test.second.xml_summary);
      tinyxml2::XMLDocument summary;
      if (summary.LoadFile(xml_fn.c_str()) != tinyxml2::XML_SUCCESS
          || !summary.FirstChild())
        continue;

      std::ifstream in(xml_fn, std::ios::binary);
      std::ostringstream ss;
      ss << in.rdbuf();
      const std::string xml_content(ss.str());

      if (!ultra::crc32::verify_xml_signature(xml_content))
        continue;

      std::lock_guard guard(current_mutex);
      test.second.current = summary_data(summary);
    }

    std::this_thread::sleep_for(3000ms);
  }
}

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

rs::settings rs::read_settings(const std::filesystem::path &test_fn)
{
  assert(test_fn.extension() == ".csv");

  const auto settings_fn(
    std::filesystem::path(test_fn).replace_extension(".xml"));

  if (!std::filesystem::exists(settings_fn))
    return {};

  tinyxml2::XMLDocument doc;
  if (const auto result(doc.LoadFile(settings_fn.c_str()));
      result != tinyxml2::XML_SUCCESS)
  {
    std::cerr << "Cannot open settings for " << test_fn << ".\n";
    return {};
  }

  settings ret;

  tinyxml2::XMLConstHandle handle(&doc);
  const auto h_ultra(handle.FirstChildElement("ultra"));

  const auto h_search(h_ultra.FirstChildElement("search"));
  if (const auto *e = h_search.FirstChildElement("generations").ToElement())
    ret.generations = e->UnsignedText(ret.generations);
  else
    ret.generations = settings::default_generations;

  if (const auto *e = h_search.FirstChildElement("runs").ToElement())
    ret.runs = e->UnsignedText(ret.runs);
  else
    ret.runs = settings::default_runs;

  if (const auto *e = h_search.FirstChildElement("threshold").ToElement())
  {
    if (const char *text = e->GetText())
      ret.threshold = extract_threshold(std::string(text));
    else
      ret.threshold = settings::default_threshold;
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

/*********************************************************************
 * Command line
 ********************************************************************/
enum class cmdl_result {error, help, monitor, run, summary};

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

[[nodiscard]] bool rs::references_available() noexcept
{
  static bool init {false};
  static bool available;

  if (!init)
  {
    for (const auto &[_, d] : collection)
      if (!d.reference.empty())
      {
        available = true;
        break;
      }

    init = true;
  }

  return available;
}

rs::collection_t rs::setup_collection(std::filesystem::path in1,
                                      std::filesystem::path in2, exec_mode m)
{
  using namespace ultra;
  namespace fs = std::filesystem;

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

  const auto check_and_insert([&m, &in2, &ret](const auto &path)
  {
    const auto get_reference([&in2](const std::filesystem::path &base)
    {
      if (!in2.empty())
      {
        if (const auto ref(in2 / summary_from_basename(base.filename()));
            std::filesystem::exists(ref))
        {
          std::cout << "\n  reference: " << ref << '\n';
          return summary_data(ref);
        }
      }

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

      const rs::data d(path, sum, read_settings(path), ref);

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

bool monitor::setup_cmd(argh::parser &cmdl)
{
  using namespace ultra;
  namespace fs = std::filesystem;

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
    std::cerr << log_folder << " isn't a directory.\n";
    return false;
  }

  monitor::slog.base_dir = log_folder;
  monitor::slog.summary_file_path = "";

  monitor::slog.dynamic_file_path =
    build_path(log_folder, cmdl("dynamic", "").str());
  monitor::slog.layers_file_path =
    build_path(log_folder, cmdl("layers", "").str());
  monitor::slog.population_file_path =
    build_path(log_folder, cmdl("population", "").str());

  std::vector<fs::path> dynamic_file_paths;
  std::vector<fs::path> layers_file_paths;
  std::vector<fs::path> population_file_paths;

  if (monitor::slog.dynamic_file_path.empty()
      || monitor::slog.layers_file_path.empty()
      || monitor::slog.population_file_path.empty())
    for (const auto &entry : fs::directory_iterator(log_folder))
      if (entry.is_regular_file()
          && ultra::iequals(entry.path().extension(), ".txt"))
      {
        const std::string fn(entry.path().filename().string());

        if (const auto def(search_log::default_dynamic_file);
            monitor::slog.dynamic_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          dynamic_file_paths.push_back(entry.path());
        }

        if (const auto def(search_log::default_layers_file);
            monitor::slog.layers_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          layers_file_paths.push_back(entry.path());
        }

        if (const auto def(search_log::default_population_file);
            monitor::slog.population_file_path.empty()
            && fn.find(def) != std::string::npos
            && (basename.empty() || fn.find(basename) != std::string::npos))
        {
          population_file_paths.push_back(entry.path());
        }

        if (dynamic_file_paths.size() > 1
            || layers_file_paths.size() > 1
            || population_file_paths.size() > 1)
        {
          const auto example(
            fs::path(entry.path()).replace_extension().replace_extension());
          std::cerr << "Too many log files in folder; please choose one (e.g."
                    << " `wopr monitor " << example << "`).\n";
          return false;
        }
      }

  if (monitor::slog.dynamic_file_path.empty() && !dynamic_file_paths.empty())
    monitor::slog.dynamic_file_path = dynamic_file_paths.front();
  if (monitor::slog.layers_file_path.empty() && !layers_file_paths.empty())
    monitor::slog.layers_file_path = layers_file_paths.front();
  if (monitor::slog.population_file_path.empty()
      && !population_file_paths.empty())
    monitor::slog.population_file_path = population_file_paths.front();

  if (!fs::exists(monitor::slog.dynamic_file_path)
      && !fs::exists(monitor::slog.layers_file_path)
      && !fs::exists(monitor::slog.population_file_path))
  {
    std::cerr << "No log file available.\n";
    return false;
  }

  std::cout << "Dynamic file path: " << monitor::slog.dynamic_file_path
            << "\nLayers file path: " << monitor::slog.layers_file_path
            << "\nPopulation file path: " << monitor::slog.population_file_path
            << '\n';


  if (const auto v(cmdl("window").str()); !v.empty())
  {
    try
    {
      monitor::window = std::stoi(v);
      std::cout << "Monitoring window: " << monitor::window << '\n';
    }
    catch (...)
    {
      std::cerr << "Wrong value for monitoring window.\n";
      return false;
    }
  }

  return true;
}

bool rs::summary::setup_cmd(argh::parser &cmdl)
{
  const auto &pos_args(cmdl.pos_args());

  if (pos_args.size() <= 2)
  {
    std::cerr << "At least one directory must be specified.\n";
    return false;
  }

  const std::filesystem::path dir1(pos_args.size() <= 2 ? "./"
                                                        : pos_args[2]);
  const std::filesystem::path dir2(pos_args.size() <= 3 ? ""
                                                        : pos_args[3]);

  collection = setup_collection(dir1, dir2, exec_mode::summary);

  if (collection.empty())
    return false;

  return true;
}

bool rs::run::setup_cmd(argh::parser &cmdl)
{
  const auto &pos_args(cmdl.pos_args());

  for (const auto &a : pos_args)
    std::cout << a << std::endl;

  const std::filesystem::path test_input(pos_args.size() <= 2 ? "./"
                                                              : pos_args[2]);
  const std::filesystem::path ref_folder(cmdl("reference", "").str());

  if (const auto v(cmdl("generations").str()); !v.empty())
  {
    settings::default_generations = std::max<unsigned>(std::stoul(v), 1);
    std::cout << "Generations: " << settings::default_generations << '\n';
  }

  if (const auto v(cmdl("runs").str()); !v.empty())
  {
    settings::default_runs = std::max<unsigned>(std::stoul(v), 1);
    std::cout << "Runs: " << settings::default_runs << '\n';
  }

  if (const auto v(cmdl("threshold").str()); !v.empty())
    settings::default_threshold = extract_threshold(v);

  collection = setup_collection(test_input, ref_folder, exec_mode::run);

  if (collection.empty())
    return false;

  run::nogui = cmdl["nogui"];

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
    return cmdl_result::help;

  std::string cmd;
  std::ranges::transform(pos_args[1], std::back_inserter(cmd),
                         [](unsigned char c){ return std::tolower(c); });

  if (!cmds.contains(cmd))
  {
    std::cerr << "Unknown command `" << cmd << "`.\n";
    return cmdl_result::error;
  }

  imgui_demo_panel = cmdl["imguidemo"];

  if (cmd == cmd_summary && rs::summary::setup_cmd(cmdl))
    return cmdl_result::summary;
  else if (cmd == cmd_monitor && monitor::setup_cmd(cmdl))
    return cmdl_result::monitor;
  else if (cmd == cmd_run && rs::run::setup_cmd(cmdl))
    return cmdl_result::run;

  return cmdl_result::error;
}

/*********************************************************************
 * MAIN
 ********************************************************************/
void monitor::start(const imgui_app::program::settings &settings)
{
  std::jthread t_logs(get_logs);

  imgui_app::program prg(settings);
  prg.run(render_monitor);
}

void rs::summary::start(const imgui_app::program::settings &settings)
{
  std::jthread t_summaries(get_summaries);

  imgui_app::program prg(settings);
  prg.run(render_rs);
}

void rs::run::start(const imgui_app::program::settings &settings)
{
  std::stop_source source;

  const auto test_driver(
    [source](auto test)
    {
      const auto &dataset(test.second.dataset);
      ultra::src::problem prob(
        ultra::src::dataframe(dataset, test.second.conf.params),
        ultra::symbol_init::all);
      prob.params.evolution.generations = test.second.conf.generations;

      ultra::src::search s(prob);

      ultra::search_log sl;
      const std::filesystem::path base_dir(dataset.parent_path());

      sl.dynamic_file_path = build_path(
        base_dir, ultra::dynamic_from_basename(dataset));
      sl.layers_file_path = build_path(
        base_dir, ultra::layers_from_basename(dataset));
      sl.population_file_path = build_path(
        base_dir, ultra::population_from_basename(dataset));
      sl.summary_file_path = test.second.xml_summary;

      s.logger(sl).stop_source(source);

      if (collection.size() > 1)
        s.tag(dataset.stem());

      return s.run(test.second.conf.runs, test.second.conf.threshold);
    });

  if (collection.size() > 1)
    ultra::log::reporting_level = ultra::log::lPAROUT;

  std::vector<std::future<ultra::search_stats<ultra::gp::individual,
                                              double>>> tasks;
  for (const auto &test : collection)
    tasks.push_back(std::async(std::launch::async, test_driver, test));

  if (run::nogui == false)
  {
    rs::summary::start(settings);

    source.request_stop();
  }

  const auto task_completed(
    [](const auto &future)
    {
      return !future.valid()
             || future.wait_for(0ms) == std::future_status::ready;
    });

  while (!std::ranges::all_of(tasks, task_completed))
    std::this_thread::sleep_for(100ms);
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
  settings.w_related.flags |= SDL_WINDOW_MAXIMIZED;
  settings.demo = imgui_demo_panel;

  if (result == cmdl_result::summary)
    rs::summary::start(settings);
  else if (result == cmdl_result::monitor)
    monitor::start(settings);
  else if (result == cmdl_result::run)
    rs::run::start(settings);

  return EXIT_SUCCESS;
}
