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

#include <cstdlib>

#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"
#include "kernel/gp/src/oracle.h"
#include "kernel/gp/src/problem.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#define TEST_WTA

// Datasets
constexpr std::size_t SR_COUNT = 10;
std::istringstream sr(R"(
      95.2425,  2.81
    1554,       6
    2866.5485,  7.043
    4680,       8
   11110,      10
   18386.0340, 11.38
   22620,      12
   41370,      14
   54240,      15
  168420,      20
)");

constexpr std::size_t IRIS_COUNT = 150;
std::istringstream iris(R"(
"setosa",5.1,3.5,1.4,0.2
"setosa",4.9,3,1.4,0.2
"setosa",4.7,3.2,1.3,0.2
"setosa",4.6,3.1,1.5,0.2
"setosa",5,3.6,1.4,0.2
"setosa",5.4,3.9,1.7,0.4
"setosa",4.6,3.4,1.4,0.3
"setosa",5,3.4,1.5,0.2
"setosa",4.4,2.9,1.4,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",5.4,3.7,1.5,0.2
"setosa",4.8,3.4,1.6,0.2
"setosa",4.8,3,1.4,0.1
"setosa",4.3,3,1.1,0.1
"setosa",5.8,4,1.2,0.2
"setosa",5.7,4.4,1.5,0.4
"setosa",5.4,3.9,1.3,0.4
"setosa",5.1,3.5,1.4,0.3
"setosa",5.7,3.8,1.7,0.3
"setosa",5.1,3.8,1.5,0.3
"setosa",5.4,3.4,1.7,0.2
"setosa",5.1,3.7,1.5,0.4
"setosa",4.6,3.6,1,0.2
"setosa",5.1,3.3,1.7,0.5
"setosa",4.8,3.4,1.9,0.2
"setosa",5,3,1.6,0.2
"setosa",5,3.4,1.6,0.4
"setosa",5.2,3.5,1.5,0.2
"setosa",5.2,3.4,1.4,0.2
"setosa",4.7,3.2,1.6,0.2
"setosa",4.8,3.1,1.6,0.2
"setosa",5.4,3.4,1.5,0.4
"setosa",5.2,4.1,1.5,0.1
"setosa",5.5,4.2,1.4,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",5,3.2,1.2,0.2
"setosa",5.5,3.5,1.3,0.2
"setosa",4.9,3.1,1.5,0.1
"setosa",4.4,3,1.3,0.2
"setosa",5.1,3.4,1.5,0.2
"setosa",5,3.5,1.3,0.3
"setosa",4.5,2.3,1.3,0.3
"setosa",4.4,3.2,1.3,0.2
"setosa",5,3.5,1.6,0.6
"setosa",5.1,3.8,1.9,0.4
"setosa",4.8,3,1.4,0.3
"setosa",5.1,3.8,1.6,0.2
"setosa",4.6,3.2,1.4,0.2
"setosa",5.3,3.7,1.5,0.2
"setosa",5,3.3,1.4,0.2
"versicolor",7,3.2,4.7,1.4
"versicolor",6.4,3.2,4.5,1.5
"versicolor",6.9,3.1,4.9,1.5
"versicolor",5.5,2.3,4,1.3
"versicolor",6.5,2.8,4.6,1.5
"versicolor",5.7,2.8,4.5,1.3
"versicolor",6.3,3.3,4.7,1.6
"versicolor",4.9,2.4,3.3,1
"versicolor",6.6,2.9,4.6,1.3
"versicolor",5.2,2.7,3.9,1.4
"versicolor",5,2,3.5,1
"versicolor",5.9,3,4.2,1.5
"versicolor",6,2.2,4,1
"versicolor",6.1,2.9,4.7,1.4
"versicolor",5.6,2.9,3.6,1.3
"versicolor",6.7,3.1,4.4,1.4
"versicolor",5.6,3,4.5,1.5
"versicolor",5.8,2.7,4.1,1
"versicolor",6.2,2.2,4.5,1.5
"versicolor",5.6,2.5,3.9,1.1
"versicolor",5.9,3.2,4.8,1.8
"versicolor",6.1,2.8,4,1.3
"versicolor",6.3,2.5,4.9,1.5
"versicolor",6.1,2.8,4.7,1.2
"versicolor",6.4,2.9,4.3,1.3
"versicolor",6.6,3,4.4,1.4
"versicolor",6.8,2.8,4.8,1.4
"versicolor",6.7,3,5,1.7
"versicolor",6,2.9,4.5,1.5
"versicolor",5.7,2.6,3.5,1
"versicolor",5.5,2.4,3.8,1.1
"versicolor",5.5,2.4,3.7,1
"versicolor",5.8,2.7,3.9,1.2
"versicolor",6,2.7,5.1,1.6
"versicolor",5.4,3,4.5,1.5
"versicolor",6,3.4,4.5,1.6
"versicolor",6.7,3.1,4.7,1.5
"versicolor",6.3,2.3,4.4,1.3
"versicolor",5.6,3,4.1,1.3
"versicolor",5.5,2.5,4,1.3
"versicolor",5.5,2.6,4.4,1.2
"versicolor",6.1,3,4.6,1.4
"versicolor",5.8,2.6,4,1.2
"versicolor",5,2.3,3.3,1
"versicolor",5.6,2.7,4.2,1.3
"versicolor",5.7,3,4.2,1.2
"versicolor",5.7,2.9,4.2,1.3
"versicolor",6.2,2.9,4.3,1.3
"versicolor",5.1,2.5,3,1.1
"versicolor",5.7,2.8,4.1,1.3
"virginica",6.3,3.3,6,2.5
"virginica",5.8,2.7,5.1,1.9
"virginica",7.1,3,5.9,2.1
"virginica",6.3,2.9,5.6,1.8
"virginica",6.5,3,5.8,2.2
"virginica",7.6,3,6.6,2.1
"virginica",4.9,2.5,4.5,1.7
"virginica",7.3,2.9,6.3,1.8
"virginica",6.7,2.5,5.8,1.8
"virginica",7.2,3.6,6.1,2.5
"virginica",6.5,3.2,5.1,2
"virginica",6.4,2.7,5.3,1.9
"virginica",6.8,3,5.5,2.1
"virginica",5.7,2.5,5,2
"virginica",5.8,2.8,5.1,2.4
"virginica",6.4,3.2,5.3,2.3
"virginica",6.5,3,5.5,1.8
"virginica",7.7,3.8,6.7,2.2
"virginica",7.7,2.6,6.9,2.3
"virginica",6,2.2,5,1.5
"virginica",6.9,3.2,5.7,2.3
"virginica",5.6,2.8,4.9,2
"virginica",7.7,2.8,6.7,2
"virginica",6.3,2.7,4.9,1.8
"virginica",6.7,3.3,5.7,2.1
"virginica",7.2,3.2,6,1.8
"virginica",6.2,2.8,4.8,1.8
"virginica",6.1,3,4.9,1.8
"virginica",6.4,2.8,5.6,2.1
"virginica",7.2,3,5.8,1.6
"virginica",7.4,2.8,6.1,1.9
"virginica",7.9,3.8,6.4,2
"virginica",6.4,2.8,5.6,2.2
"virginica",6.3,2.8,5.1,1.5
"virginica",6.1,2.6,5.6,1.4
"virginica",7.7,3,6.1,2.3
"virginica",6.3,3.4,5.6,2.4
"virginica",6.4,3.1,5.5,1.8
"virginica",6,3,4.8,1.8
"virginica",6.9,3.1,5.4,2.1
"virginica",6.7,3.1,5.6,2.4
"virginica",6.9,3.1,5.1,2.3
"virginica",5.8,2.7,5.1,1.9
"virginica",6.8,3.2,5.9,2.3
"virginica",6.7,3.3,5.7,2.5
"virginica",6.7,3,5.2,2.3
"virginica",6.3,2.5,5,1.9
"virginica",6.5,3,5.2,2
"virginica",6.2,3.4,5.4,2.3
"virginica",5.9,3,5.1,1.8
)");

template<template<class> class L, ultra::IndividualOrTeam T, unsigned P>
struct build
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d, P);
  }
};

template<template<class> class L, ultra::IndividualOrTeam T>
struct build<L, T, 0>
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d);
  }
};

template<ultra::IndividualOrTeam T>
struct build<ultra::src::reg_oracle, T, 0>
{
  ultra::src::reg_oracle<T> operator()(const T &prg,
                                       ultra::src::dataframe &) const
  {
    return ultra::src::reg_oracle(prg);
  }
};

template<template<class> class L, ultra::IndividualOrTeam T, unsigned P = 0>
void test_serialization(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned cycles(256); cycles; --cycles)
  {
    const T ind(pr);
    const auto oracle1(build<L, T, P>()(ind, pr.data()));

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load<T>(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data())
    {
      const auto out1(oracle1.name(oracle1(e)));
      const auto out2(oracle2->name((*oracle2)(e)));

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
    const auto li(build<L, gp::individual, P>()(ind, pr.data()));

    const team<gp::individual> t{{ind}};
    const auto lt(build<L, team<gp::individual>, P>()(t, pr.data()));

    for (const auto &e : pr.data())
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

    const auto oracle1(build<L, gp::individual, P>()(ind1, pr.data()));
    const auto oracle2(build<L, gp::individual, P>()(ind2, pr.data()));
    const auto oracle3(build<L, gp::individual, P>()(ind3, pr.data()));

    const team<gp::individual> t{{ind1, ind2, ind3}};
    const auto ts(t.size());
    const auto oracle_t(build<L, team<gp::individual>, P>()(t, pr.data()));

    for (const auto &example : pr.data())
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

  CHECK(pr.data().read_csv(sr) == SR_COUNT);
  pr.setup_symbols();

  // TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::reg_oracle>(pr);

  // TEAM OF IDENTICAL INDIVIDUALS.
  for (unsigned i(0); i < 1000; ++i)
  {
    const gp::individual ind(pr);
    const src::reg_oracle li(ind);

    const team<gp::individual> t{{ind, ind, ind, ind}};
    const src::reg_oracle lt(t);

    for (const auto &e : pr.data())
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

    const team<gp::individual> t({i1, i2, i3, i4});
    const src::reg_oracle oracle_team(t);

    for (const auto &e : pr.data())
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

        if (std::fabs(sum / n) < 0.000001)
          CHECK(almost_equal(std::get<D_DOUBLE>(out_t), 0.0));
        else
        {
          if (!almost_equal(sum / n, std::get<D_DOUBLE>(out_t)))
            std::cout << std::get<D_DOUBLE>(out1) << "  "
                      << std::get<D_DOUBLE>(out2) << "  "
                      << std::get<D_DOUBLE>(out3) << "  "
                      << std::get<D_DOUBLE>(out4) << "       "
                      << std::get<D_DOUBLE>(out_t) << std::endl;
          CHECK(sum / n == doctest::Approx(std::get<D_DOUBLE>(out_t)));
        }
      }
    }
  }
}

TEST_CASE_FIXTURE(fixture, "reg_oracle serialization")
{
  using namespace ultra;

  CHECK(pr.data().read_csv(sr) == SR_COUNT);
  pr.setup_symbols();

  for (unsigned iterations(1000); iterations; --iterations)
  {
    const gp::individual ind(pr);
    const src::reg_oracle oracle1(ind);

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data())
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

  CHECK(pr.data().read_csv(iris) == IRIS_COUNT);
  pr.setup_symbols();

  // GAUSSIAN ORACLE TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::gaussian_oracle>(pr);

  // GAUSSIAN ORACLE TEAM OF RANDOM INDIVIDUALS.
  test_team<src::gaussian_oracle>(pr);
}

}  // TEST_SUITE("ORACLE")
