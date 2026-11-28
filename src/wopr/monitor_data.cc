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

#include "monitor_data.h"

#include "kernel/exceptions.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>
#include <utility>

namespace ultra::wopr
{

dynamic_data::dynamic_data(const std::string &line) : new_run(line.empty())
{
  if (!new_run)
  {
    ultra::fitnd fit_min_val;
    std::istringstream ss(line);
    if (!(ss >> generation)
        || !(ss >> fit_best)
        || !(ss >> fit_mean)
        || !(ss >> fit_std_dev)
        || !(ss >> fit_min_val)
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

bool dynamic_sequence::empty() const noexcept
{
  return xs.empty();
}

std::size_t dynamic_sequence::size() const noexcept
{
  return xs.size();
}

void dynamic_sequence::push_back(const dynamic_data &dd)
{
  xs.push_back(dd.generation);
  fit_best.push_back(dd.fit_best[0]);
  fit_mean.push_back(dd.fit_mean[0]);
  fit_std_dev.push_back(dd.fit_std_dev[0]);
  len_mean.push_back(dd.len_mean);
  len_std_dev.push_back(dd.len_std_dev);
  len_max.push_back(dd.len_max);

  // Assume zero crossovers for every known type.
  for (auto &[_, history] : crossover_types)
    history.push_back(0);

  for (const auto &[type, count] : dd.crossover_types)
  {
    auto &history(crossover_types[type]);

    // Backfills previous generations when this type is new.
    history.resize(xs.size(), 0);

    // Possibly replace a zero with the value actually reported.
    history.back() = count;
  }

  if (best_prg.empty() || best_prg.back() != dd.best_prg)
    best_prg.push_back(dd.best_prg);
}

population_line::population_line(const std::string &line)
  : new_run(line.empty())
{
  if (new_run)
    return;

  std::istringstream ss(line);
  if (!(ss >> generation))
    throw ultra::exception::data_format(
      "Cannot parse population file line (missing generation)");

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

bool population_sequence::empty() const noexcept
{
  return fit.empty();
}

void population_sequence::update(population_line &pl)
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
double population_sequence::calculate_entropy() const
{
  const auto pop_size(static_cast<double>(
    std::accumulate(obs.begin(), obs.end(), std::size_t{0})));
  if (pop_size <= 0.0)
    return 0.0;

  double h(0.0);
  for (const auto x : obs)
    if (x > 0)
    {
      const auto p(static_cast<double>(x) / pop_size);

      h -= p * std::log2(p);
    }

  return h;
}

layers_line::layers_line(const std::string &line) : new_run(line.empty())
{
  if (new_run)
    return;

  std::istringstream ss(line);
  if (!(ss >> generation))
    throw ultra::exception::data_format(
      "Cannot parse layers file line (missing generation)");

  decltype(age_sup)::value_type age_sup_val;
  decltype(age_mean)::value_type age_mean_val;
  double age_std_dev_val;
  decltype(age_min)::value_type age_min_val;
  decltype(age_max)::value_type age_max_val;
  ultra::fitnd fit_mean_val;
  ultra::fitnd fit_std_dev_val;
  ultra::fitnd fit_min_val;
  ultra::fitnd fit_max_val;
  decltype(individuals)::value_type individuals_val;

  while (ss >> age_sup_val)
  {
    if (!(ss >> age_mean_val >> age_std_dev_val >> age_min_val >> age_max_val
             >> fit_mean_val >> fit_std_dev_val >> fit_min_val >> fit_max_val
             >> individuals_val))
      throw ultra::exception::data_format(
        "Cannot parse layers file line (missing layer data)");

    age_sup.push_back(age_sup_val);
    age_mean.push_back(age_mean_val);
    age_min.push_back(age_min_val);
    age_max.push_back(age_max_val);

    fit_mean.push_back(fit_mean_val[0]);
    fit_std_dev.push_back(fit_std_dev_val[0]);
    fit_min.push_back(fit_min_val[0]);
    fit_max.push_back(fit_max_val[0]);

    individuals.push_back(individuals_val);
  }
}

bool layers_line::empty() const noexcept
{
  return age_sup.empty();
}

std::size_t layers_line::size() const noexcept
{
  return age_sup.size();
}

layers_fit_matrix build_layers_fit_matrix(const layers_line &lr)
{
  layers_fit_matrix out;
  if (lr.empty())
    return out;

  const auto max_layers(lr.size());
  const std::size_t ind_max(std::ranges::max(lr.individuals));
  if (!ind_max)
    return out;

  const std::size_t parts(std::min<std::size_t>(ind_max, 100));
  const double fit_max(std::ranges::max(lr.fit_max));
  const double fit_min(std::ranges::min(lr.fit_min));

  out.rows = max_layers;
  out.cols = parts;
  out.ind_max = ind_max;
  out.fit_min = fit_min;
  out.fit_max = fit_max;
  out.values.assign(max_layers * parts, fit_min);

  std::minstd_rand g;
  g.seed(lr.generation);

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
        out.values[layer * parts + p] = val;
    }
    else
    {
      std::normal_distribution dist(mean, stddev);

      for (std::size_t p(0); p < full; ++p)
        out.values[layer * parts + p] = std::clamp(dist(g), min, max);
    }
  }

  return out;
}

/// Appends a parsed log record, separating runs at empty lines.
template<class Data, class Run, class Update>
void push_run_entry(Data &&data, std::vector<Run> &runs, Update update)
{
  if (data.new_run)
  {
    // Skip multiple empty lines.
    if (runs.empty() || !runs.back().empty())
      runs.push_back({});
  }
  else
  {
    if (runs.empty())
      runs.push_back({});

    update(runs.back(), std::forward<Data>(data));
  }
}

void monitor_state::push(const dynamic_data &data)
{
  push_run_entry(data, dynamic_runs,
                 [](auto &run, const auto &entry) { run.push_back(entry); });
}

void monitor_state::push(population_line data)
{
  push_run_entry(std::move(data), population_runs,
                 [](auto &run, auto &&entry) { run.update(entry); });
}

void monitor_state::push(layers_line data)
{
  push_run_entry(std::move(data), layers_runs,
                 [](auto &run, auto &&entry) { run = std::move(entry); });
}

}  // namespace ultra::wopr
