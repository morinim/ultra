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
 *  \see https://github.com/morinim/ultra/wiki/symbolic_regression_part4
 */

/* CHANGES IN THIS FILE MUST BE APPLIED TO THE LINKED WIKI PAGE */

#include "kernel/ultra.h"

constexpr std::size_t    N(6);
constexpr std::size_t VARS(3);

struct example
{
  example(const std::vector<double> &ex_a, const ultra::matrix<double> &ex_b,
          const std::vector<double> &ex_x)
    : a(ex_a), b(ex_b)
  {
    std::ranges::copy(ex_x, std::back_inserter(x));
  }

  std::vector<double>         a;
  ultra::matrix<double>       b;
  std::vector<ultra::value_t> x {};
};

using training_set = std::vector<example>;

training_set get_training_set()
{
  const auto get_vector([](std::size_t s)
  {
    std::vector<double> v(s);
    std::ranges::generate(v,
                          [] { return ultra::random::sup(10000.0); });
    return v;
  });

  const auto get_matrix([]
  {
    ultra::matrix<double> m(N, N);
    std::ranges::generate(m,
                          [] { return ultra::random::between(-10.0, 10.0); });
    return m;
  });

  training_set ret;

  std::generate_n(std::back_inserter(ret), 1000,
                  [get_vector, get_matrix]
                  {
                    return example(get_vector(N),
                                   get_matrix(),
                                   get_vector(VARS));
                  });

  return ret;
}

using candidate_solution = ultra::gp::team<ultra::gp::individual>;

class error_functor
{
public:
  error_functor(const candidate_solution &s) : s_(s) {}

  double operator()(const example &ex) const
  {
    using namespace ultra;

    std::vector<double> f(N);
    std::ranges::transform(s_, f.begin(),
                   [&ex](const auto &prg)
                   {
                     const auto ret(run(prg, ex.x));

                     return has_value(ret) ? std::get<D_DOUBLE>(ret) : 0.0;
                   });

    std::vector<double> model(N, 0.0);
    for (unsigned i(0); i < N; ++i)
      for (unsigned j(0); j < N; ++j)
        model[i] += ex.b(i, j) * f[j];

    double delta(std::inner_product(ex.a.begin(), ex.a.end(),
                                    model.begin(), 0.0,
                                    std::plus{},
                                    [](auto v1, auto v2)
                                    {
                                      return std::fabs(v1 - v2);
                                    }));

    return delta;
  }

private:
  candidate_solution s_;
};

// Given a team (i.e. a candidate solution of the problem), returns a score
// measuring how good it performs on a given dataset.
class my_evaluator
  : public ultra::src::sum_of_errors_evaluator<candidate_solution,
                                               error_functor, training_set>
{
public:
  explicit my_evaluator(training_set &d)
    : sum_of_errors_evaluator<candidate_solution, error_functor,
                              training_set>(d)
  {}
};

int main()
{
  using namespace ultra;

  training_set data(get_training_set());

  problem prob;
  prob.params.team.individuals = N;

  // SETTING UP SYMBOLS
  prob.sset.insert<src::variable>(0, "x1");
  prob.sset.insert<src::variable>(1, "x2");
  prob.sset.insert<src::variable>(2, "x3");
  prob.insert<real::number>();
  prob.insert<real::add>();
  prob.insert<real::sub>();
  prob.insert<real::mul>();

  // AD HOC EVALUATOR
  my_evaluator eva(data);
  search s(prob, eva);

  // SEARCHING
  const auto result(s.run());

  std::cout << "\nCANDIDATE SOLUTION\n"
            << out::c_language << result.best_individual()
            << "\n\nFITNESS\n" << *result.best_measurements().fitness << '\n';
}
