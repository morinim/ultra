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

#include "kernel/random.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/evaluator.h"
#include "kernel/gp/src/problem.h"

#include "utility/timer.h"

#include "test/fixture1.h"

#include <execution>
#include <future>

ultra::src::dataframe make_dataset(std::size_t nr)
{
  using namespace ultra;
  src::dataframe d;

  d.set_schema({{"Y", d_double},
                {"X1", d_double}, {"X2", d_double},
                {"X3", d_double}, {"X4", d_double}});

  for (std::size_t i(0); i < nr; ++i)
  {
    src::example ex;

    ex.input = {random::sup(1000.0), random::sup(1000.0),
                random::sup(1000.0), random::sup(1000.0)};
    ex.output = random::sup(1000.0);

    d.push_back(ex);
  }

  return d;
}

double standard_sum(const ultra::src::dataframe &d,
                    const ultra::gp::individual &ind)
{
  const ultra::src::mae_error_functor ef(ind);

  const auto errf([&ef](const auto &v)
  {
    return std::clamp<double>(ef(v), -10000, 10000);
  });

  auto it(std::begin(d));
  auto average_error(errf(*it));
  ++it;

  double n(1.0);

  while (it != std::end(d))
  {
    average_error += (errf(*it) - average_error) / ++n;

    ++it;
  }

  return -average_error;
}

double par_reduce_sum(const ultra::src::dataframe &d,
                      const ultra::gp::individual &ind)
{
  struct partial_mean
  {
    double mean {0.0};
    std::size_t count {0};
  };

  const auto stride(
    std::max(1zu,
             std::min<std::size_t>(
               std::thread::hardware_concurrency(),
               d.size() / 10)));

  std::vector<std::future<partial_mean>> futures;
  futures.reserve(stride);

  for (std::size_t i(0); i < stride; ++i)
    futures.emplace_back(
      std::async(
        std::launch::async,
        [&ind, &d, stride](std::size_t start)
        {
          // Thread-local functor.
          const ultra::src::mae_error_functor ef(ind);

          const auto errf([&ef](const auto &v)
          {
            return std::clamp<double>(ef(v), -10000, 10000);
          });

          const auto end(d.end());
          auto it(std::ranges::next(d.begin(), start, end));

          partial_mean pm;

          if (it == end)
            return pm;

          pm.mean = errf(*it);
          pm.count = 1;

          std::ranges::advance(it, stride, end);

          while (it != end)
          {
            pm.mean += (errf(*it) - pm.mean) / static_cast<double>(++pm.count);

            std::ranges::advance(it, stride, end);
          }

          return pm;
        }, i));

  // Reduction (weighted, numerically stable)
  double avg(0.0);
  std::size_t total(0);

  for (auto &f : futures)
  {
    const auto pm(f.get());

    if (pm.count != 0)
    {
      const auto new_total(total + pm.count);

      avg +=
        (pm.mean - avg)
        * (static_cast<double>(pm.count) / static_cast<double>(new_total));

      total = new_total;
    }
  }

  return -avg;
}

double par_reduce_pairwise_sum(const ultra::src::dataframe &d,
                               const ultra::gp::individual &ind)
{
  struct partial_sum
  {
    double sum {0.0};
    std::size_t count {0};
  };

  // Recursive pairwise summation on [start, end[ with thread-local errf.
  const auto pairwise_sum([&d, &ind](std::size_t start, std::size_t end)
  {
    const ultra::src::mae_error_functor ef(ind);
    const auto errf([&ef](const auto &v)
    {
      return std::clamp<double>(ef(v), -10000, 10000);
    });

    // Recursive pairwise sum
    const auto recur_sum([&d, &errf](this auto && self,
                                     std::size_t s, std::size_t e)
    {
      if (s >= e) return 0.0;
      if (s + 1 == e) return errf(*std::next(d.begin(), s));

      const std::size_t mid(s + (e - s) / 2);
      return self(s, mid) + self(mid, e);
    });

    return recur_sum(start, end);
  });

  // Split dataset in two halves for two threads
  const std::size_t mid(d.size() / 2);

  auto f1(std::async(std::launch::async, pairwise_sum, 0, mid));
  auto f2(std::async(std::launch::async, pairwise_sum, mid, d.size()));

  const double sum1(f1.get());
  const double sum2(f2.get());

  const double total_sum(sum1 + sum2);

  return -total_sum / static_cast<double>(d.size());
}


double par_reduce_kahan_sum(const ultra::src::dataframe &d,
                            const ultra::gp::individual &ind)
{
  // Threshold below which we use a single thread
  constexpr std::size_t SINGLE_THREAD_THRESHOLD = 1000;

  // Lambda for numerically stable Kahan summation on [start, end[
  const auto pairwise_sum([&d, &ind](std::size_t start, std::size_t end)
  {
    const ultra::src::mae_error_functor ef(ind);
    const auto errf([&ef](const auto &v)
    {
      return std::clamp<double>(ef(v), -10000, 10000);
    });

    double sum(0.0);
    double c(0.0);  // Kahan compensation

    for (std::size_t i(start); i < end; ++i)
    {
      double y = errf(*std::next(d.begin(), i)) - c;
      double t = sum + y;
      c = (t - sum) - y;
      sum = t;
    }

    return sum;
  });

  // Decide number of threads.
  if (d.size() < SINGLE_THREAD_THRESHOLD)
  {
    // Single-threaded
    return -pairwise_sum(0, d.size()) / static_cast<double>(d.size());
  }

  // Two-threaded for larger datasets.
  const std::size_t mid = d.size() / 2;

  auto f1 = std::async(std::launch::async, pairwise_sum, 0, mid);
  auto f2 = std::async(std::launch::async, pairwise_sum, mid, d.size());

  double sum1 = f1.get();
  double sum2 = f2.get();

  // Combine results with Kahan for numerical stability
  double total = 0.0;
  double c = 0.0;
  for (double x : {sum1, sum2})
  {
    double y = x - c;
    double t = total + y;
    c = (t - total) - y;
    total = t;
  }

  return -total / static_cast<double>(d.size());
}



int main()
{
  using namespace ultra;

  constexpr std::size_t DATASETS {3};

  const std::vector ds{make_dataset(  100),
                       make_dataset( 1000),
                       make_dataset(10000)};

  assert(ds.size() == DATASETS);

  src::problem prob(ds.front());
  prob.params.init();

  prob.insert<real::sin>();
  prob.insert<real::cos>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::div>();
  prob.insert<real::mul>();

  std::vector<gp::individual> individuals;
  for (std::size_t i(0); i < 400; ++i)
    individuals.emplace_back(prob);

  const std::string s1("Algorithm / Examples     ");

  const std::size_t data_field(10);

  std::cout << s1;

  for (unsigned i(0); i < ds.size(); ++i)
    std::cout << std::setw(data_field + 2) << ds[i].size();

  double out[DATASETS];
  {
    std::vector<double> elapsed;

    for (unsigned i(0); const auto &d : ds)
    {
      ultra::timer t;
      for (const auto &ind : individuals)
        out[i] += standard_sum(d, ind);

      elapsed.push_back(t.elapsed().count());

      ++i;
    }

    std::cout << '\n' << std::left << std::setw(s1.size()) << "Standard sum";
    for (auto e : elapsed)
      std::cout << std::right << std::setw(data_field) << e << "ms";
  }

  // -------------------------------------------------------------------------

  volatile double out1[DATASETS];
  {
    std::vector<double> elapsed;

    for (unsigned i(0); const auto &d : ds)
    {
      ultra::timer t;
      for (const auto &ind : individuals)
        out1[i] += par_reduce_sum(d, ind);

      elapsed.push_back(t.elapsed().count());

      ++i;
    }

    std::cout << '\n' << std::left << std::setw(s1.size()) << "Parallel sum";
    for (auto e : elapsed)
      std::cout << std::right << std::setw(data_field) << e << "ms";
  }

  // -------------------------------------------------------------------------

  volatile double out2[DATASETS];
  {
    std::vector<double> elapsed;

    for (unsigned i(0); const auto &d : ds)
    {
      ultra::timer t;
      for (const auto &ind : individuals)
        out2[i] += par_reduce_pairwise_sum(d, ind);

      elapsed.push_back(t.elapsed().count());

      ++i;
    }

    std::cout << '\n' << std::left << std::setw(s1.size())
              << "Parallel pairwise sum";
    for (auto e : elapsed)
      std::cout << std::right << std::setw(data_field) << e << "ms  ";
  }

  // -------------------------------------------------------------------------

  volatile double out3[DATASETS];
  {
    std::vector<double> elapsed;

    for (unsigned i(0); const auto &d : ds)
    {
      ultra::timer t;
      for (const auto &ind : individuals)
        out3[i] += par_reduce_kahan_sum(d, ind);

      elapsed.push_back(t.elapsed().count());

      ++i;
    }

    std::cout << '\n' << std::left << std::setw(s1.size())
              << "Parallel Kahan sum";
    for (auto e : elapsed)
      std::cout << std::right << std::setw(data_field) << e << "ms  ";
  }

  // -------------------------------------------------------------------------

  std::cout << "\n\n\n";
  for (unsigned i(0); i < ds.size(); ++i)
  {
    std::cout << out[i]
              << "  " << out1[i]
              << "  " << out2[i]
              << "  " << out3[i]
              << '\n';
  }
}
