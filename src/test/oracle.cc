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

// Examples in dataset
constexpr std::size_t SR_COUNT        =  10;

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

  for (unsigned k(0); k < 256; ++k)
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
      const auto out_i(li(e)), out_t(lt(e));

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
      const auto out_i(li(e)), out_t(lt(e));

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
      const auto out1(oracle1(e));
      const auto out2(oracle2(e));
      const auto out3(oracle3(e));
      const auto out4(oracle4(e));

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
        const auto out_t(oracle_team(e));

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
/*
TEST_CASE_FIXTURE(fixture, "reg_oracle serialization")
{
  using namespace ultra;

  CHECK(pr.data().read("./test_resources/mep.csv") == MEP_COUNT);
  pr.setup_symbols();

  for (unsigned k(0); k < 1000; ++k)
  {
    const i_mep ind(pr);
    const reg_lambda_f<i_mep> lambda1(ind);

    std::stringstream ss;

    CHECK(serialize::save(ss, lambda1));
    const auto lambda2(serialize::lambda::load(ss, pr.sset));
    REQUIRE(lambda2);
    REQUIRE(lambda2->is_valid());

    for (const auto &e : pr.data())
    {
      const auto out1(lambda1(e));
      const auto out2((*lambda2)(e));

      if (has_value(out1))
        CHECK(lexical_cast<D_DOUBLE>(out1)
              == doctest::Approx(lexical_cast<D_DOUBLE>(out2)));
      else
        CHECK(!has_value(out2));
    }
  }
}

template<template<class> class L, unsigned P = 0>
void test_team(ultra::src_problem &pr)
{
  using namespace ultra;

  for (unsigned i(0); i < 1000; ++i)
  {
    const i_mep ind1(pr);
    const i_mep ind2(pr);
    const i_mep ind3(pr);

    const auto lambda1(build<L, i_mep, P>()(ind1, pr.data()));
    const auto lambda2(build<L, i_mep, P>()(ind2, pr.data()));
    const auto lambda3(build<L, i_mep, P>()(ind3, pr.data()));

    const team<i_mep> t{{ind1, ind2, ind3}};
    const auto ts(t.individuals());
    const auto lambda_t(build<L, team<i_mep>, P>()(t, pr.data()));

    for (const auto &example : pr.data())
    {
      const std::vector out =
      {
        lambda1(example), lambda2(example), lambda3(example)
      };
      const std::vector<std::string> names =
      {
        lambda1.name(out[0]), lambda2.name(out[1]), lambda3.name(out[2])
      };
      const std::vector<classification_result> tags =
      {
        lambda1.tag(example), lambda2.tag(example), lambda3.tag(example)
      };

      for (auto j(decltype(ts){0}); j < ts; ++j)
        CHECK(std::get<D_INT>(out[j]) == tags[j].label);

      std::string s_best(names[0]);

#if defined(TEST_MV)
      std::map<std::string, unsigned> votes;

      for (auto j(decltype(ts){0}); j < ts; ++j)
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
      class_t c_best(0);

      for (auto j(decltype(ts){1}); j < ts; ++j)
        if (tags[j].sureness > tags[c_best].sureness)
        {
          s_best = names[j];
          c_best = j;
        }
#endif

      CHECK(s_best == lambda_t.name(lambda_t(example)));
    }
  }
}
*/
}  // TEST_SUITE("ORACLE")
