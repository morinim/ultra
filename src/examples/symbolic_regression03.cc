/*
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 *
 *  \see https://github.com/morinim/ultra/wiki/symbolic_regression_part2
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

const double a = ultra::random::between(-10.0, 10.0);
const double b = ultra::random::between(-10.0, 10.0);

class c : public ultra::terminal
{
public:
  c() : ultra::terminal("c") {}

  [[nodiscard]] ultra::value_t instance() const noexcept final
  {
    static const double val(ultra::random::between(-10.0, 10.0));
    return val;
  }
};

using candidate_solution = ultra::gp::individual;

// Given an individual (i.e. a candidate solution of the problem), returns a
// score measuring how good it is.
[[nodiscard]] double my_evaluator(const candidate_solution &x)
{
  using namespace ultra;

  const auto ret(run(x));

  const double f(has_value(ret) ? std::get<D_DOUBLE>(ret) : 0.0);

  const double model_output(b * f);

  const double delta(std::fabs(a - model_output));

  return -delta;
}

int main()
{
  using namespace ultra;

  problem prob;

  // SETTING UP SYMBOLS
  prob.insert<c>();          // terminal
  //prob.insert<real::literal>(random::between(-10.0, 10.0));
  prob.insert<real::add>();  // functions
  prob.insert<real::sub>();
  prob.insert<real::mul>();

  // AD HOC EVALUATOR
  search s(prob, my_evaluator);

  // SEARCHING
  const auto result(s.run());

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual
            << "\n\nFITNESS\n" << *result.best_measurements.fitness << '\n';
}
