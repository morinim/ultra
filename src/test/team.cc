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
#include <sstream>

#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("TEAM")
{

TEST_CASE("Concept")
{
  using namespace ultra::gp;

  REQUIRE(ultra::Individual<team<individual>>);

  REQUIRE(Team<team<individual>>);
  REQUIRE(!Team<individual>);
}

TEST_CASE_FIXTURE(fixture1, "Random creation")
{
  using namespace ultra::gp;

  // Variable length random creation
  for (auto l(prob.sset.categories() + 2); l < 100; ++l)
  {
    prob.params.slp.code_length = l;
    team<individual> t(prob);

    CHECK(t.is_valid());
    CHECK(t.age() == 0);
  }
}

TEST_CASE_FIXTURE(fixture1, "Mutation")
{
  using namespace ultra::gp;

  prob.params.slp.code_length = 100;

  team<individual> t(prob);
  const auto orig(t);

  CHECK(t.size() > 0);

  const unsigned n(4000);

  SUBCASE("Zero probability mutation")
  {
    prob.params.evolution.p_mutation = 0.0;
    for (unsigned i(0); i < n; ++i)
    {
      t.mutation(prob);
      CHECK(t == orig);
    }
  }

  SUBCASE("50% probability mutation")
  {
    double diff(0.0), length(0.0);

    prob.params.evolution.p_mutation = 0.5;

    for (unsigned i(0); i < n; ++i)
    {
      const team<individual> t1{t};

      t.mutation(prob);
      diff += distance(t, t1);
      length += active_slots(t1);
    }

    const double perc(100.0 * diff / length);
    CHECK(perc >= 45.0);
    CHECK(perc <= 55.0);
  }
}

TEST_CASE_FIXTURE(fixture1, "Comparison")
{
  using namespace ultra::gp;

  for (unsigned i(0); i < 2000; ++i)
  {
    team<individual> a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == 0);

    team<individual> b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    team<individual> c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(a != c);
      CHECK(distance(a, c) > 0);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Iterators")
{
  using namespace ultra::gp;

  for (unsigned j(0); j < 1000; ++j)
  {
    team<individual> t(prob);

    for (unsigned i(0); const auto &ind : t)
    {
      CHECK(ind == t[i]);
      ++i;
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Crossover")
{
  using namespace ultra;

  prob.params.slp.code_length = 100;

  gp::team<gp::individual> t1(prob), t2(prob);

  const unsigned n(2000);
  for (unsigned j(0); j < n; ++j)
  {
    const auto tc(crossover(t1, t2));
    CHECK(tc.is_valid());

    for (std::size_t p(0); p < tc.size(); ++p)
      for (locus::index_t i(0); i < tc[p].size(); ++i)
        for (symbol::category_t c(0); c < tc[p].categories(); ++c)
        {
          const locus l{i, c};

          CHECK((tc[p][l] == t1[p][l] || tc[p][l] == t2[p][l]));
        }
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  for (unsigned i(0); i < 2000; ++i)
  {
    std::stringstream ss;
    gp::team<gp::individual> t1(prob);

    t1.inc_age(random::sup(100u));

    CHECK(t1.save(ss));

    gp::team<gp::individual> t2(prob);
    CHECK(t2.load(ss, prob.sset));
    CHECK(t2.is_valid());

    CHECK(t1 == t2);
  }
}

}  // TEST_SUITE("TEAM")
