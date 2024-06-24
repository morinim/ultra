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
 *  \see https://github.com/morinim/ultra/wiki/symbolic_regression_part3
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

constexpr std::size_t N(6);

std::vector<double> get_vector()
{
  std::vector<double> ret(N);

  std::ranges::generate(ret,
                        [] { return ultra::random::between(-10.0, 10.0); });

  return ret;
}

ultra::matrix<double> get_matrix()
{
  ultra::matrix<double> ret(N, N);

  std::ranges::generate(ret,
                        [] { return ultra::random::between(-10.0, 10.0); });

  return ret;
}

const auto a = get_vector();  // N-dimensional vector
const auto b = get_matrix();  // NxN matrix

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

using candidate_solution = ultra::gp::team<ultra::gp::individual>;

// Given a team (i.e. a candidate solution of the problem), returns a score
// measuring how good it is.
[[nodiscard]] double my_evaluator(const candidate_solution &x)
{
  using namespace ultra;

  std::vector<double> f(N);
  std::ranges::transform(
    x, f.begin(),
    [](const auto &prg)
    {
      const auto ret(run(prg));

      return has_value(ret) ? std::get<D_DOUBLE>(ret) : 0.0;
    });

  std::vector<double> model(N, 0.0);
  for (unsigned i(0); i < N; ++i)
    for (unsigned j(0); j < N; ++j)
      model[i] += b(i, j) * f[j];

  double delta(std::inner_product(a.begin(), a.end(), model.begin(), 0.0,
                                  std::plus{},
                                  [](auto v1, auto v2)
                                  {
                                    return std::fabs(v1 - v2);
                                  }));

  return -delta;
}

int main()
{
  using namespace ultra;

  problem prob;

  prob.params.team.individuals = N;

  // SETTING UP SYMBOLS
  prob.sset.insert<c>();
  prob.insert<real::add>();
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
