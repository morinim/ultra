/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "test/debug_datasets.h"

#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/oracle.h"
#include "kernel/gp/src/problem.h"
#include "kernel/gp/src/variable.h"

#include <cstdlib>
#include <future>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#define TEST_WTA

template<template<class> class L, ultra::Individual T, unsigned P>
struct build
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d, P);
  }
};

template<template<class> class L, ultra::Individual T>
struct build<L, T, 0>
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d);
  }
};

template<ultra::Individual T>
struct build<ultra::src::reg_oracle, T, 0>
{
  ultra::src::reg_oracle<T> operator()(const T &prg,
                                       ultra::src::dataframe &) const
  {
    return ultra::src::reg_oracle(prg);
  }
};

template<template<class> class L, ultra::Individual T, unsigned P = 0>
void test_serialization(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned cycles(256); cycles; --cycles)
  {
    const T ind(pr);
    const auto oracle1(build<L, T, P>()(ind, pr.data.selected()));

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load<T>(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1.name(oracle1(e.input)));
      const auto out2(oracle2->name((*oracle2)(e.input)));

      CHECK(out1 == out2);
    }
  }
}

template<template<class> class L, unsigned P = 0>
void test_team_of_one(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned i(0); i < 1000; ++i)
  {
    const gp::individual ind(pr);
    const auto li(build<L, gp::individual, P>()(ind, pr.data.selected()));

    const gp::team<gp::individual> t{{ind}};
    const auto lt(build<L, gp::team<gp::individual>, P>()(t,
                                                          pr.data.selected()));

    for (const auto &e : pr.data.selected())
    {
      const auto out_i(li(e.input)), out_t(lt(e.input));

      if (has_value(out_i))
      {
        const auto v1(lexical_cast<D_DOUBLE>(out_i));
        const auto v2(lexical_cast<D_DOUBLE>(out_t));

        CHECK(almost_equal(v1, v2));
      }
      else
        CHECK(!has_value(out_t));
    }
  }
}

template<template<class> class L, unsigned P = 0>
void test_team(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual ind1(pr);
    const gp::individual ind2(pr);
    const gp::individual ind3(pr);

    const auto oracle1(build<L, gp::individual, P>()(ind1, pr.data.selected()));
    const auto oracle2(build<L, gp::individual, P>()(ind2, pr.data.selected()));
    const auto oracle3(build<L, gp::individual, P>()(ind3, pr.data.selected()));

    const gp::team<gp::individual> t{{ind1, ind2, ind3}};
    const auto ts(t.size());
    const auto oracle_t(build<L, gp::team<gp::individual>, P>()(
                          t, pr.data.selected()));

    for (const auto &example : pr.data.selected())
    {
      const std::vector out =
      {
        oracle1(example.input), oracle2(example.input), oracle3(example.input)
      };
      const std::vector<std::string> names =
      {
        oracle1.name(out[0]), oracle2.name(out[1]), oracle3.name(out[2])
      };
      const std::vector<src::classification_result> tags =
      {
        oracle1.tag(example.input), oracle2.tag(example.input),
        oracle3.tag(example.input)
      };

      for (std::size_t j(0); j < ts; ++j)
        CHECK(std::get<D_INT>(out[j]) == tags[j].label);

      std::string s_best(names[0]);

#if defined(TEST_MV)
      std::map<std::string, unsigned> votes;

      for (std::size_t j(0); j < ts; ++j)
      {
        if (votes.find(names[j]) == votes.end())
          votes[names[j]] = 1;
        else
          ++votes[names[j]];
      }

      unsigned v_best(0);

      for (auto &v : votes)
        if (v.second > v_best)
        {
          s_best = v.first;
          v_best = v.second;
        }
#elif defined(TEST_WTA)
      src::class_t c_best(0);

      for (std::size_t j(1); j < ts; ++j)
        if (tags[j].sureness > tags[c_best].sureness)
        {
          s_best = names[j];
          c_best = j;
        }
#endif

      CHECK(s_best == oracle_t.name(oracle_t(example.input)));
    }
  }
}

struct fixture
{
  fixture() { pr.params.init(); }

  ultra::src::problem pr {};
};

TEST_SUITE("ORACLE")
{

TEST_CASE_FIXTURE(fixture, "reg_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::sr);
  CHECK(pr.data.selected().read(is) == debug::SR_COUNT);
  pr.setup_symbols();

  // TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::reg_oracle>(pr);

  // TEAM OF IDENTICAL INDIVIDUALS.
  for (unsigned i(0); i < 1000; ++i)
  {
    const gp::individual ind(pr);
    const src::reg_oracle li(ind);

    const gp::team<gp::individual> t{{ind, ind, ind, ind}};
    const src::reg_oracle lt(t);

    for (const auto &e : pr.data.selected())
    {
      const auto out_i(li(e.input)), out_t(lt(e.input));

      if (has_value(out_i))
      {
        const auto v1(std::get<D_DOUBLE>(out_i));
        const auto v2(std::get<D_DOUBLE>(out_t));

        CHECK(almost_equal(v1, v2));
      }
      else
        CHECK(!has_value(out_t));
    }
  }

  // TEAM OF RANDOM INDIVIDUALS.
  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual i1(pr);
    const gp::individual i2(pr);
    const gp::individual i3(pr);
    const gp::individual i4(pr);

    const src::reg_oracle oracle1(i1);
    const src::reg_oracle oracle2(i2);
    const src::reg_oracle oracle3(i3);
    const src::reg_oracle oracle4(i4);

    const gp::team<gp::individual> t({i1, i2, i3, i4});
    const src::reg_oracle oracle_team(t);

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1(e.input));
      const auto out2(oracle2(e.input));
      const auto out3(oracle3(e.input));
      const auto out4(oracle4(e.input));

      D_DOUBLE sum(0.0), n(0.0);
      if (has_value(out1))
      {
        sum += lexical_cast<D_DOUBLE>(out1);
        ++n;
      }
      if (has_value(out2))
      {
        sum += lexical_cast<D_DOUBLE>(out2);
        ++n;
      }
      if (has_value(out3))
      {
        sum += lexical_cast<D_DOUBLE>(out3);
        ++n;
      }
      if (has_value(out4))
      {
        sum += lexical_cast<D_DOUBLE>(out4);
        ++n;
      }

      if (n > 0.0)
      {
        const auto out_t(oracle_team(e.input));

        if (!almost_equal(sum / n, std::get<D_DOUBLE>(out_t)))
        {
          ultraWARNING << std::get<D_DOUBLE>(out1) << "  "
                       << std::get<D_DOUBLE>(out2) << "  "
                       << std::get<D_DOUBLE>(out3) << "  "
                       << std::get<D_DOUBLE>(out4) << "       "
                       << sum / n << " "
                       << std::get<D_DOUBLE>(out_t) << std::endl;
        }

        CHECK(sum / n == doctest::Approx(std::get<D_DOUBLE>(out_t)));
      }
    }
  }
}


TEST_CASE_FIXTURE(fixture, "reg_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::sr);
  CHECK(pr.data.selected().read(is) == debug::SR_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual ind(pr);
    const src::reg_oracle oracle1(ind);

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1(e.input));
      const auto out2((*oracle2)(e.input));

      if (has_value(out1))
        CHECK(std::get<D_DOUBLE>(out1)
              == doctest::Approx(std::get<D_DOUBLE>(out2)));
      else
        CHECK(!has_value(out2));
    }
  }
}

TEST_CASE_FIXTURE(fixture, "gaussian_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::iris_full);
  CHECK(pr.data.selected().read(is) == debug::IRIS_FULL_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // GAUSSIAN ORACLE TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::gaussian_oracle>(pr);

  // GAUSSIAN ORACLE TEAM OF RANDOM INDIVIDUALS.
  test_team<src::gaussian_oracle>(pr);
}

TEST_CASE_FIXTURE(fixture, "gaussian_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::iris_full);
  CHECK(pr.data.selected().read(is) == debug::IRIS_FULL_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // GAUSSIAN_ORACLE SERIALIZATION - INDIVIDUAL.
  test_serialization<src::gaussian_oracle, gp::individual>(pr);

  // GAUSSIAN_ORACLE SERIALIZATION - TEAM.
  test_serialization<src::gaussian_oracle, gp::team<gp::individual>>(pr);
}

TEST_CASE_FIXTURE(fixture, "binary_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender);
  CHECK(pr.data.selected().read(is) == debug::GENDER_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // BINARY LAMBDA TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::binary_oracle>(pr);

  // BINARY LAMBDA TEAM OF RANDOM INDIVIDUALS.
  test_team<src::binary_oracle>(pr);
}

TEST_CASE_FIXTURE(fixture, "binary_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender);
  CHECK(pr.data.selected().read(is) == debug::GENDER_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // BINARY_LAMBDA_F SERIALIZATION - INDIVIDUAL.
  test_serialization<src::binary_oracle, gp::individual>(pr);

  // BINARY_LAMBDA_F SERIALIZATION - TEAM.
  test_serialization<src::binary_oracle, gp::team<gp::individual>>(pr);
}

TEST_CASE_FIXTURE(fixture, "Perfect binary_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender_trick);
  CHECK(pr.data.selected().read(is) == debug::GENDER_TRICK_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  const auto *easy(static_cast<const src::variable *>(
                     pr.sset.decode("EASY")));
  CHECK(easy);

  const function *f_add(pr.insert<ultra::real::add>());

  const gp::individual delphi(
    {
      {f_add, {easy, easy}}
    });

  const src::binary_oracle oracle(delphi, pr.data.selected());
  for (const auto &e : pr.data.selected())
    CHECK(oracle.tag(e.input).label == label(e));
}

TEST_CASE("Parallel oracles")
{
  using namespace ultra;

  src::dataframe df;

  df.set_schema({{"Y", d_int},
                 {"X1", d_int}, {"X2", d_int}, {"X3", d_int}, {"X4", d_int}});

  for (std::size_t i(0); i < 1000; ++i)
  {
    src::example ex;

    ex.input = {random::sup<D_INT>(1000), random::sup<D_INT>(1000),
                random::sup<D_INT>(1000), random::sup<D_INT>(1000)};
    ex.output = random::sup<D_INT>(1000);

    df.push_back(ex);
  }

  src::problem prob(df);
  prob.params.init();

  prob.insert<integer::add>();
  prob.insert<integer::sub>();
  prob.insert<integer::div>();
  prob.insert<integer::mul>();

  std::vector<gp::individual> individuals;
  for (std::size_t i(0); i < 200; ++i)
    individuals.emplace_back(prob);

  const auto standard_sum([](const ultra::src::dataframe &d,
                             const ultra::gp::individual &ind)
  {
    src::reg_oracle oracle(ind);

    int sum(0.0);
    for (const auto &in : d)
      if (const auto v(oracle(in.input)); has_value(v))
        sum += std::get<D_INT>(v);

    return sum;
  });

  const auto par_reduce_sum([](const ultra::src::dataframe &d,
                               const ultra::gp::individual &ind)
  {
    const auto stride(
      std::max<std::size_t>(1, std::thread::hardware_concurrency()));

    std::vector<std::future<int>> futures;
    futures.reserve(stride);

    for (std::size_t i(0); i < stride; ++i)
      futures.emplace_back(
        std::async(
          std::launch::async,
          [&ind, &d, stride](std::size_t start)
          {
            // Thread-local oracle.
            src::reg_oracle oracle(ind);

            const auto end(d.end());
            auto it(std::ranges::next(d.begin(), start, end));

            int ps(0);

            if (it == end)
              return ps;

            if (const auto v(oracle(it->input)); has_value(v))
              ps = std::get<D_INT>(v);

            std::ranges::advance(it, stride, end);

            while (it != end)
            {
              if (const auto v(oracle(it->input)); has_value(v))
                ps += std::get<D_INT>(v);

              std::ranges::advance(it, stride, end);
            }

            return ps;
          }, i));

    // Reduction.
    int sum(0);

    for (auto &f : futures)
    {
      const auto ps(f.get());
      sum += ps;
    }

    return sum;
  });

  for (const auto &ind : individuals)
    CHECK(standard_sum(df, ind) == par_reduce_sum(df, ind));
}

}  // TEST_SUITE("ORACLE")
