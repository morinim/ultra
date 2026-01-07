/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/hga/individual.h"

#include "test/fixture6.h"

#include <cstdlib>
#include <sstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("hga::individual")
{

TEST_CASE_FIXTURE(fixture6, "Random creation")
{
  using namespace ultra;

  for (unsigned cycles(1000); cycles; --cycles)
  {
    hga::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(!ind.empty());
    CHECK(ind.parameters() == prob.sset.categories());
    CHECK(ind.age() == 0);

    for (std::size_t i(0); i < ind.parameters(); ++i)
    {
      const auto *ft(prob.sset.front_terminal(i));

      if (const auto *ti = get_if<hga::integer>(ft))
      {
        const auto val(std::get<D_INT>(ind[i]));
        CHECK(ti->min() <= val);
        CHECK(val < ti->sup());
      }
      else if (const auto *tp = get_if<hga::permutation>(ft))
      {
        D_IVECTOR base(tp->length());
        std::iota(base.begin(), base.end(), 0);

        CHECK(std::ranges::is_permutation(std::get<D_IVECTOR>(ind[i]), base));
      }
    }
  }
}

TEST_CASE_FIXTURE(fixture6, "Empty individual")
{
  ultra::hga::individual ind;

  CHECK(ind.is_valid());
  CHECK(ind.empty());
}

TEST_CASE_FIXTURE(fixture6, "Mutation")
{
  ultra::hga::individual t(prob);
  const auto orig(t);

  const unsigned n(1000);

  SUBCASE("Zero probability mutation")
  {
    prob.params.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < n; ++i)
    {
      t.mutation(prob);
      CHECK(t == orig);
    }
  }

  SUBCASE("Mutation sequences")
  {
    std::vector<double> sequence;
    for (double pgm(0.1); pgm <= 1.0; pgm += 0.1)
    {
      prob.params.evolution.p_mutation = pgm;

      unsigned diff(0);
      for (unsigned i(0); i < n; ++i)
      {
        auto i1(orig);

        i1.mutation(prob);
        diff += distance(orig, i1);
      }

      sequence.push_back(diff);
    }

    CHECK(std::ranges::is_sorted(sequence));
  }
}

TEST_CASE_FIXTURE(fixture6, "Comparison")
{
  using namespace ultra;

  for (unsigned cycles(2000); cycles; --cycles)
  {
    hga::individual a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == 0);

    auto b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    hga::individual c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(!(a == c));
      CHECK(distance(a, c) > 0);
      CHECK(distance(a, c) == distance(c, a));
    }
  }
}

TEST_CASE("Vector assignment")
{
  using namespace ultra;

  hga::problem prob;
  prob.params.init();

  prob.insert<hga::permutation>(3);
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));

  hga::individual a(prob), a1;
  a.modify([](auto &m)
  {
    m[0] = std::vector{0, 1, 2};
    m[1] = 0;
    m[2] = 1;
    m[3] = 2;
  });

  CHECK(a != a1);

  a1 = {std::vector{0, 1, 2}, 0, 1, 2};

  CHECK(a == a1);
  CHECK(a.signature() == a1.signature());
}

TEST_CASE("Distance")
{
  using namespace ultra;

  hga::problem prob;
  prob.params.init();

  prob.insert<hga::permutation>(3);
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));

  hga::individual a, b;

  a = {std::vector{0, 1, 2}, 0, 1, 2};
  b = {std::vector{1, 0, 2}, 0, 2, 2};

  CHECK(distance(a, b) == 3);
}

TEST_CASE_FIXTURE(fixture6, "Iterators")
{
  for (unsigned cycles(1000); cycles; --cycles)
  {
    ultra::hga::individual ind(prob);

    for (unsigned i(0); const auto &g : ind)
      CHECK(g == ind[i++]);
  }
}

TEST_CASE_FIXTURE(fixture6, "Modify")
{
  using namespace ultra;

  hga::problem prob;
  prob.params.init();

  prob.insert<hga::permutation>(3);
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));
  prob.insert<hga::integer>(interval(0, 9));

  hga::individual a(prob), a1;

  CHECK(a != a1);
  CHECK(a.signature() != a1.signature());
  CHECK(a1.signature().empty());

  a1.modify([&a](auto &m)
  {
    std::ranges::copy(a, std::back_inserter(m.genome()));
  });

  CHECK(a == a1);
  CHECK(a.signature() == a1.signature());
}

TEST_CASE_FIXTURE(fixture6, "Standard crossover")
{
  using namespace ultra;

  hga::individual i1(prob), i2(prob);

  for (unsigned cycles(1000); cycles; --cycles)
  {
    if (random::boolean())
      i1.inc_age();
    if (random::boolean())
      i2.inc_age();

    const auto ic(crossover(prob, i1, i2));
    CHECK(ic.is_valid());
    CHECK(ic.age() == std::max(i1.age(), i2.age()));

    const auto d1(distance(i1, ic));
    CHECK(0 <= d1);
    CHECK(d1 <= fixture6::actual_length);

    const auto d2(distance(i2, ic));
    CHECK(0 <= d2);
    CHECK(d1 <= fixture6::actual_length);

    for (std::size_t k(0); k < ic.parameters(); ++k)
    {
      const auto *t(prob.sset.front_terminal(k));

      if (is<hga::integer>(t))
      {
        const bool from_1_or_2(ic[k] == i1[k] || ic[k] == i2[k]);
        CHECK(from_1_or_2);
      }
      else if (is<hga::permutation>(t))
        CHECK(std::ranges::is_permutation(std::get<D_IVECTOR>(ic[k]),
                                          std::get<D_IVECTOR>(i1[k])));
    }
  }
}

TEST_CASE_FIXTURE(fixture6, "Serialization")
{
  using namespace ultra;

  SUBCASE("Non-empty hga::individual serialization")
  {
    for (unsigned cycles(2000); cycles; --cycles)
    {
      std::stringstream ss;
      hga::individual i1(prob);

      i1.inc_age(ultra::random::sup(100));

      CHECK(i1.save(ss));

      hga::individual i2(prob);
      CHECK(i2.load(ss));
      CHECK(i2.is_valid());

      CHECK(i1 == i2);
    }
  }

  SUBCASE("Empty hga::individual serialization")
  {
    std::stringstream ss;
    hga::individual empty;
    CHECK(empty.save(ss));

    hga::individual empty1;
    CHECK(empty1.load(ss));
    CHECK(empty1.is_valid());
    CHECK(empty1.empty());

    CHECK(empty == empty1);
  }
}

TEST_CASE_FIXTURE(fixture6, "Signature")
{
  using namespace ultra;

  hga::individual i1(prob), i2(i1);

  CHECK(i1.signature() == i2.signature());
  i1.modify([](auto &m) { std::swap(m[0], m[m.size() - 1]); });
  CHECK(i1.signature() != i2.signature());
  i1.modify([](auto &m) { std::swap(m[0], m[m.size() - 1]); });
  CHECK(i1.signature() == i2.signature());

  auto vec(std::get<D_IVECTOR>(i1[0]));
  std::ranges::next_permutation(vec);

  i1.modify([&vec](auto &m) { m[0] = vec; });
  CHECK(i1.signature() != i2.signature());
}

}  // TEST_SUITE("hga::individual")
