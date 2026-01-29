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

#include <algorithm>

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

std::pair<ultra::src::dataframe, std::vector<ultra::gp::individual>>
make_env(const ultra::src::problem &prob,
         std::size_t examples, std::size_t population)
{
  auto d(make_dataset(examples));

  std::vector<ultra::gp::individual> individuals;
  individuals.reserve(population);

  for (std::size_t i(0); i < population; ++i)
    individuals.emplace_back(prob);

  return {d, individuals};
}

int main()
{
  using namespace ultra;

  const std::vector<std::pair<std::size_t, std::size_t>> env =
  {
    { 100, 100}, { 100, 1000}, { 100, 10000}, { 100, 30000}, { 100, 50000},
    { 500, 100}, { 500, 1000}, { 500, 10000}, { 500, 30000}, { 500, 50000},
    {1000, 100}, {1000, 1000}, {1000, 10000}, {1000, 30000}, {1000, 50000},
    {5000, 100}, {5000, 1000}, {5000, 10000}, {5000, 30000}, {5000, 50000}
  };

  // Volatile sinks are used to prevent the compiler from eliding the
  // evaluation loops.
  volatile double out(0), out1(0), out2(0), out3(0);

  src::problem prob(make_dataset(1));
  prob.params.init();

  prob.insert<ultra::real::sin>();
  prob.insert<ultra::real::cos>();
  prob.insert<ultra::real::add>();
  prob.insert<ultra::real::sub>();
  prob.insert<ultra::real::div>();
  prob.insert<ultra::real::mul>();

  std::cout << std::string(14, ' ')
            << "Embedded     On the fly       Rebind      Rebind optional\n";

  for (const auto &e : env)
  {
    const auto &[d, pop] = make_env(prob, e.first, e.second);

    std::vector<src::interpreter> interpreters;
    interpreters.reserve(pop.size());

    std::ranges::transform(
      pop, std::back_inserter(interpreters),
      [](const auto &ind) { return src::interpreter(ind); });

    std::cout << '(' << std::setw(4) << e.first
              << ',' << std::setw(5) << e.second
              << ")  ";

    ultra::timer t;
    for (const auto &example : d)
      for (auto &intr : interpreters)
        if (const auto v(intr.run(example.input)); has_value(v))
          out += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);

    std::cout << std::setw(6) << t.elapsed().count() << "ms      "
              << std::flush;

    // -----------------------------------------------------------------------

    t.restart();
    for (const auto &example : d)
      for (const auto &ind : pop)
      {
        src::interpreter intr(ind);
        if (const auto v(intr.run(example.input)); has_value(v))
          out1 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
      }

    std::cout << std::setw(7) << t.elapsed().count() << "ms      "
              << std::flush;

    // -----------------------------------------------------------------------

    t.restart();

    for (const auto &example : d)
      for (std::size_t i(0); auto &intr : interpreters)
      {
        intr.rebind(pop[i++]);
        if (const auto v(intr.run(example.input)); has_value(v))
          out2 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
      }

    std::cout << std::setw(5) << t.elapsed().count() << "ms      "
              << std::flush;

    // -----------------------------------------------------------------------

    std::vector<std::optional<src::interpreter>> ointerpreters(pop.size());

    t.restart();

    for (const auto &example : d)
      for (std::size_t i(0); auto &ointr : ointerpreters)
      {
        if (ointr) [[likely]]
          ointr->rebind(pop[i]);
        else
          ointr.emplace(pop[i]);

        ++i;

        if (const auto v(ointr->run(example.input)); has_value(v))
          out3 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
      }

    std::cout << std::setw(13) << t.elapsed().count() << "ms\n";
  }

  std::cout << out << "    " << out1 << "    " << out2 << "    " << out3
            << "\n";

  return !std::isinf(out + out1 + out2 + out3);  // just to stop some warnings
}
