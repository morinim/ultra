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

int main()
{
  using namespace ultra;

  const auto d(make_dataset(20000));

  src::problem prob(d);
  prob.params.init();

  prob.insert<real::sin>();
  prob.insert<real::cos>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::div>();
  prob.insert<real::mul>();

  const std::size_t INDIVIDUALS(1000);
  std::vector<gp::individual> individuals;
  std::vector<src::interpreter> interpreters;

  individuals.reserve(INDIVIDUALS);
  interpreters.reserve(INDIVIDUALS);

  for (std::size_t i(0); i < INDIVIDUALS; ++i)
  {
    individuals.emplace_back(prob);
    interpreters.emplace_back(individuals.back());
  }

  // -------------------------------------------------------------------------

  volatile double out(0);

  ultra::timer t;
  for (const auto &example : d)
    for (auto &intr : interpreters)
    {
      if (const auto v(intr.run(example.input)); has_value(v))
        out += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
    }

  std::cout << "Embedded interpreter - Elapsed: " << t.elapsed().count()
            << "ms\n";

  // -------------------------------------------------------------------------

  volatile double out1(0);

  t.restart();
  for (const auto &example : d)
    for (const auto &ind : individuals)
    {
      src::interpreter intr(ind);
      if (const auto v(intr.run(example.input)); has_value(v))
        out1 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
    }

  std::cout << "On the fly interpreter - Elapsed: " << t.elapsed().count()
            << "ms\n";

  // -------------------------------------------------------------------------

  volatile double out2(0);

  t.restart();

  src::interpreter intr(individuals.front());

  for (const auto &example : d)
    for (const auto &ind : individuals)
    {
      intr.rebind(ind);
      if (const auto v(intr.run(example.input)); has_value(v))
        out2 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
    }

  std::cout << "Rebind interpreter - Elapsed: " << t.elapsed().count()
            << "ms\n";
  // -------------------------------------------------------------------------

  volatile double out3(0);

  t.restart();

  std::optional<src::interpreter> ointr(individuals.front());

  for (const auto &example : d)
    for (const auto &ind : individuals)
    {
      if (ointr) [[likely]]
        ointr->rebind(ind);
      else
        ointr.emplace(ind);

      if (const auto v(ointr->run(example.input)); has_value(v))
        out3 += std::clamp(std::get<D_DOUBLE>(v), -10.0, 10.0);
    }

  std::cout << "Rebind optional interpreter - Elapsed: " << t.elapsed().count()
            << "ms\n";

  // -------------------------------------------------------------------------
  std::cout << out << "    " << out1 << "    " << out2 << "    " << out3
            << "\n";

  return !std::isinf(out + out1 + out2);  // just to stop some warnings
}
