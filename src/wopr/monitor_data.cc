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

#include <cmath>
#include <iostream>
#include <numbers>
#include <numeric>
#include <sstream>
#include <utility>

namespace ultra::wopr
{

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
  xs.push_back(xs.size());
  fit_best.push_back(dd.fit_best[0]);
  fit_mean.push_back(dd.fit_mean[0]);
  fit_std_dev.push_back(dd.fit_std_dev[0]);
  len_mean.push_back(dd.len_mean);
  len_std_dev.push_back(dd.len_std_dev);
  len_max.push_back(dd.len_max);

  // Assume zero crossovers for every known type.
  for (auto &[_, history] : crossover_types)
    history.push_back(0.0);

  for (const auto &[type, count] : dd.crossover_types)
  {
    auto &history(crossover_types[type]);

    // Backfills previous generations when this type is new.
    history.resize(xs.size(), 0.0);

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

bool population_sequence::empty() const noexcept
{
  return fit.empty();
}

std::size_t population_sequence::size() const noexcept
{
  return fit.size();
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
  const auto sum(std::accumulate(obs.begin(), obs.end(), 0uz));
  if (!sum)
    return 0;

  constexpr double c(1.0 / std::numbers::ln2_v<double>);
  const auto pop_size(static_cast<double>(sum));

  double h(0.0);
  for (auto x : obs)
    if (x > 0.0)
    {
      const auto p(x / pop_size);

      h -= p * std::log(p) * c;
    }

  return h;
}

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

bool layers_sequence::empty() const noexcept
{
  return age_sup.empty();
}

std::size_t layers_sequence::size() const noexcept
{
  return age_sup.size();
}

void layers_sequence::update(layers_line &ld)
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

}  // namespace ultra::wopr
