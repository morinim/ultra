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
 *
 *  Speed test to evaluate parallel aggregate evaluation threshold.
 */

#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/evaluator.h"
#include "kernel/gp/src/problem.h"
#include "kernel/gp/src/variable.h"
#include "utility/log.h"
#include "utility/misc.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace ultra;

src::dataframe make_bench_dataset(std::size_t rows)
{
  constexpr std::size_t cols(4);
  src::dataframe d;
  std::vector<std::pair<std::string, domain_t>> schema = {{"Y", d_double}};
  for (std::size_t c(1); c <= cols; ++c)
    schema.emplace_back("X" + std::to_string(c), d_double);
  d.set_schema(schema);

  for (std::size_t r(0); r < rows; ++r)
  {
    src::example ex;
    for (std::size_t c(0); c < cols; ++c)
      ex.input.emplace_back(0.25
                            + static_cast<double>((r * (c + 3)) % 97) / 97.0);
    ex.output = 0.5 + static_cast<double>(r % 31) / 31.0;
    d.push_back(ex);
  }

  return d;
}

int main()
{
  log::reporting_level = log::lWARNING;

  const std::array<std::size_t, 4> dataset_sizes {512, 2048, 10000, 25000};
  const std::array<std::size_t, 4> thresholds {128, 256, 512, 1024};
  const unsigned max_helpers(static_cast<unsigned>(
    std::clamp(hardware_threads(), 2uz, 8uz) - 1));

  src::problem prob(make_bench_dataset(1));
  prob.params.init();
  const function *add(prob.insert<real::add>());
  const function *mul(prob.insert<real::mul>());
  const function *sin(prob.insert<real::sin>());
  const auto *x1(static_cast<const src::variable *>(prob.sset.decode("X1")));
  const auto *x2(static_cast<const src::variable *>(prob.sset.decode("X2")));
  const auto *x3(static_cast<const src::variable *>(prob.sset.decode("X3")));
  const auto *x4(static_cast<const src::variable *>(prob.sset.decode("X4")));
  Expects(x1 && x2 && x3 && x4);

  const gp::individual cheap({{add, {x1, 0.0}}});
  const gp::individual expensive({
    {sin, {x1}},
    {mul, {0_addr, x2}},
    {sin, {1_addr}},
    {mul, {2_addr, x3}},
    {sin, {3_addr}},
    {add, {4_addr, x4}}
  });

  struct result
  {
    std::string_view program;
    bool fast;
    std::size_t rows;
    unsigned helpers;
    std::size_t threshold;
    std::size_t workers;
    double serial_us;
    double speedup;
  };
  std::vector<result> results;

  for (auto rows : dataset_sizes)
  {
    src::multi_dataset<src::dataframe> dat;
    dat[src::dataset_t::training] = make_bench_dataset(rows);

    src::mae_evaluator<gp::individual> eva(dat);

    for (const auto &[program_name, ind] :
           std::array{std::pair{"cheap", &cheap},
                      std::pair{"costly", &expensive}})
    {
      for (const bool fast : {false, true})
      {
        volatile double sink(0.0);

        const auto evaluate([&]
        {
          sink += fast ? eva.fast(*ind) : eva(*ind);
        });

        const auto measure_ms([&](std::size_t evaluations)
        {
          evaluate();
          const auto start(std::chrono::steady_clock::now());
          for (std::size_t i(0); i < evaluations; ++i)
            evaluate();
          return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        });

        eva.set_parallel_evaluation(0);
        const double calibration_ms(measure_ms(32));
        const auto evaluations(std::clamp<std::size_t>(
          static_cast<std::size_t>(32.0 * 50.0 / std::max(0.001, calibration_ms)),
          32, 5000));

        for (unsigned helper_case(0);
             helper_case < (max_helpers == 1 ? 1u : 2u); ++helper_case)
        {
          const unsigned helpers(helper_case ? max_helpers : 1u);

          for (auto threshold : thresholds)
          {
            constexpr std::size_t repeats(5);
            std::array<double, repeats> ratios;
            std::array<double, repeats> serial_times;
            const auto time_config = [&](unsigned h, std::size_t th)
            {
              eva.set_parallel_evaluation(h, th);
              return measure_ms(evaluations);
            };

            for (std::size_t repeat(0); repeat < repeats; ++repeat)
            {
              double serial_ms, parallel_ms;

              if (repeat % 2)
              {
                parallel_ms = time_config(helpers, threshold);
                serial_ms = time_config(0, 0);
              }
              else
              {
                serial_ms = time_config(0, 0);
                parallel_ms = time_config(helpers, threshold);
              }

              serial_times[repeat] = serial_ms;
              ratios[repeat] = serial_ms / parallel_ms;
            }

            std::ranges::nth_element(ratios, ratios.begin() + repeats / 2);
            std::ranges::nth_element(serial_times,
                                     serial_times.begin() + repeats / 2);

            const std::size_t samples(fast
              ? (rows - 1) / src::mae_evaluator<gp::individual>::fast_step + 1
              : rows);
            const auto workers(std::max(1uz,
              std::min(static_cast<std::size_t>(helpers) + 1,
                       samples / threshold)));

            results.push_back({program_name, fast, rows, helpers, threshold,
                               workers,
                               1000.0 * serial_times[repeats / 2] / evaluations,
                               ratios[repeats / 2]});
          }
        }
      }
    }
  }

  const auto find_result([&](std::string_view program, bool fast,
                             unsigned helpers, std::size_t rows,
                             std::size_t threshold) -> const result &
  {
    const auto it(std::ranges::find_if(results, [&](const result &r)
    {
      return r.program == program && r.fast == fast && r.helpers == helpers
             && r.rows == rows && r.threshold == threshold;
    }));
    Expects(it != results.end());
    return *it;
  });

  std::cout << "Parallel aggregate evaluation; default threshold: "
            << src::mae_evaluator<gp::individual>::default_min_samples_per_thread
            << " samples per worker\n"
            << "Cells show workers/speedup.\n";

  for (const std::string_view program : {"cheap", "costly"})
    for (const bool fast : {false, true})
      for (unsigned helper_case(0);
           helper_case < (max_helpers == 1 ? 1u : 2u); ++helper_case)
      {
        const unsigned helpers(helper_case ? max_helpers : 1u);
        std::cout << "\n" << program << ", " << (fast ? "fast" : "full")
                  << ", " << helpers << (helpers == 1 ? " helper\n" : " helpers\n")
                  << std::setw(8) << "Rows" << " | "
                  << std::setw(14) << "Serial us/eval";
        for (auto threshold : thresholds)
          std::cout << " | " << std::setw(10)
                    << ("th=" + std::to_string(threshold));
        std::cout << '\n';

        for (auto rows : dataset_sizes)
        {
          const auto &baseline(find_result(program, fast, helpers, rows,
                                           thresholds.front()));
          std::cout << std::setw(8) << rows << " | " << std::setw(14)
                    << std::fixed << std::setprecision(1) << baseline.serial_us;

          for (auto threshold : thresholds)
          {
            const auto &r(find_result(program, fast, helpers, rows, threshold));
            std::cout << " | " << std::setw(4) << r.workers << '/'
                      << std::fixed << std::setprecision(2)
                      << std::setw(4) << r.speedup << 'x';
          }
          std::cout << '\n';
        }
      }
}
