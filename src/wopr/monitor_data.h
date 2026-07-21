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

#if !defined(ULTRA_WOPR_MONITOR_DATA_H)
#define      ULTRA_WOPR_MONITOR_DATA_H

#include "kernel/fitness.h"
#include "kernel/individual.h"

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace ultra::wopr
{

using crossover_types_map = std::map<int, unsigned>;

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

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

  void push_back(const dynamic_data &);
};

struct population_line
{
  explicit population_line(const std::string &);

  bool new_run {false};
  unsigned generation {0};

  std::vector<double> fit {};
  std::vector<double> obs {};
};

struct population_sequence
{
  std::vector<double> fit {};
  std::vector<double> obs {};

  std::vector<double> xs {};
  std::vector<double> fit_entropy {};

  unsigned generation {0};

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

  void update(population_line &);
  [[nodiscard]] double calculate_entropy() const;
};

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

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

  void update(layers_line &);
};

}  // namespace ultra::wopr

#endif  // include guard
