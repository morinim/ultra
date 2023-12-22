/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <sstream>

#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture2.h"
#include "test/fixture3.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("GP INDIVIDUAL")
{

TEST_CASE_FIXTURE(fixture1, "Random creation")
{
  using namespace ultra;

  // Variable length random creation.
  for (auto l(1); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(ind.size() == l);
    CHECK(!ind.empty());
    CHECK(ind.age() == 0);

    for (locus::index_t i(0); i < ind.size(); ++i)
      for (symbol::category_t c(0); c < prob.sset.categories(); ++c)
      {
        CHECK(ind[{i, c}].category() == c);

        for (const auto &a : ind[{i, c}].args)
          if (const auto *pa(std::get_if<param_address>(&a)); pa)
            CHECK(as_integer(*pa) < i);
      }
  }
}

TEST_CASE_FIXTURE(fixture2, "Random creation multicategories")
{
  using namespace ultra;

  // Variable length random creation.
  for (auto l(1); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(ind.size() == l);
    CHECK(!ind.empty());
    CHECK(ind.age() == 0);

    for (locus::index_t i(0); i < ind.size(); ++i)
      for (symbol::category_t c(0); c < prob.sset.categories(); ++c)
        if (prob.sset.functions(c))
        {
          CHECK(ind[{i, c}].category() == c);

          for (const auto &a : ind[{i, c}].args)
            if (const auto *pa(std::get_if<param_address>(&a)); pa)
              CHECK(as_integer(*pa) < i);
        }
  }
}

TEST_CASE_FIXTURE(fixture3, "Random creation full-multicategories")
{
  using namespace ultra;

  // Variable length random creation.
  for (auto l(1); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual ind(prob);

    CHECK(ind.is_valid());
    CHECK(ind.size() == l);
    CHECK(!ind.empty());
    CHECK(ind.age() == 0);

    for (locus::index_t i(0); i < ind.size(); ++i)
      for (symbol::category_t c(0); c < prob.sset.categories(); ++c)
        if (prob.sset.functions(c))
        {
          CHECK(ind[{i, c}].category() == c);

          for (const auto &a : ind[{i, c}].args)
            if (const auto *pa(std::get_if<param_address>(&a)); pa)
              CHECK(as_integer(*pa) < i);
        }
  }
}

TEST_CASE_FIXTURE(fixture1, "Construction from vector")
{
  using namespace ultra;

  gp::individual i({
                     {f_add, {3.0, 2.0}},       // [0] ADD 3.0 2.0
                     {f_add, {0_addr, 1.0}},    // [1] ADD [0] 1.0
                     {f_sub, {1_addr, 0_addr}}  // [2] SUB [1] [0]
                   });

  CHECK(i.is_valid());
  CHECK(i.size() == 3);
  CHECK(!i.empty());
  CHECK(i.age() == 0);

  CHECK(i[{0, 0}].category() == symbol::default_category);
  CHECK(i[{1, 0}].category() == symbol::default_category);
  CHECK(i[{2, 0}].category() == symbol::default_category);

  CHECK(i[{0, 0}].func == f_add);
  CHECK(i[{1, 0}].func == f_add);
  CHECK(i[{2, 0}].func == f_sub);

  CHECK(i[{2, 0}].args == gene::arg_pack{1_addr, 0_addr});
}

TEST_CASE_FIXTURE(fixture1, "Iterators")
{
  using namespace ultra;

  // Variable length random creation.
  for (auto l(1); l < 100; ++l)
  {
    prob.env.slp.code_length = l;
    gp::individual ind(prob);

    SUBCASE("Exons")
    {
      for (const auto &g : ind.cexons())
        CHECK(g.is_valid());

      auto exons(ind.cexons());
      locus previous(locus::npos());
      for (auto it(exons.begin()); it != exons.end(); ++it)
      {
        CHECK(it.locus() < previous);
        previous = it.locus();
      }
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Comparison")
{
  using namespace ultra;

  for (unsigned i(0); i < 2000; ++i)
  {
    gp::individual a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == 0);

    gp::individual b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    gp::individual c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(a != c);
      CHECK(distance(a, c) > 0);
      CHECK(distance(a, c) == distance(c, a));
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Signature")
{
  using namespace ultra;

  gp::individual i({
                     {f_add, {3.0, 2.0}},       // [0] ADD 3.0 2.0
                     {f_add, {0_addr, 1.0}},    // [1] ADD [0] 1.0
                     {f_sub, {1_addr, 0_addr}}  // [2] SUB [1] [0]
                   });

  gp::individual eq1({
                       {f_add, {3.0, 2.0}},       // [0] ADD 3.0 2.0
                       {f_add, {4.0, 5.0}},       // [1] ADD 4.0 5.0
                       {f_add, {0_addr, 1.0}},    // [2] ADD [0] 1.0
                       {f_sub, {2_addr, 0_addr}}  // [3] SUB [2] [0]
                     });

  gp::individual eq2({
                       {f_add, {7.0, 9.0}},       // [0] ADD 7.0 9.0
                       {f_add, {3.0, 2.0}},       // [1] ADD 3.0 2.0
                       {f_add, {4.0, 5.0}},       // [2] ADD 4.0 5.0
                       {f_add, {1_addr, 1.0}},    // [3] ADD [1] 1.0
                       {f_sub, {3_addr, 1_addr}}  // [4] SUB [3] [1]
                     });

  gp::individual neq1({
                        {f_add, {3.0, 2.0}},       // [0] ADD 3.0 2.0
                        {f_add, {1.0, 0_addr}},    // [1] ADD 1.0 [0]
                        {f_sub, {1_addr, 0_addr}}  // [2] SUB [1] [0]
                      });

  gp::individual neq2({
                        {f_add, {3.0, 2.0}},       // [0] ADD 3.0 2.0
                        {f_add, {0_addr, 1.0}},    // [1] ADD [0] 1.0
                        {f_sub, {0_addr, 1_addr}}  // [2] SUB [1] [0]
                      });

  CHECK(i.signature() == eq1.signature());
  CHECK(i.signature() == eq2.signature());

  CHECK(i.signature() != neq1.signature());
  CHECK(i.signature() != neq2.signature());
}

TEST_CASE_FIXTURE(fixture1, "Mutation")
{
  using namespace ultra;

  prob.env.slp.code_length = 100;

  gp::individual ind(prob);
  const gp::individual orig(ind);

  const unsigned n(4000);

  SUBCASE("Zero probability mutation")
  {
    prob.env.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < n; ++i)
    {
      ind.mutation(prob);
      CHECK(ind == orig);
    }
  }

  SUBCASE("Random probability")
  {
    for (unsigned j(0); j < 10; ++j)
    {
      prob.env.evolution.p_mutation = random::between(0.1, 0.9);
      unsigned total_length(0), total_mut(0);

      for (unsigned i(0); i < n; ++i)
      {
        const gp::individual i1(ind);

        const auto mut(ind.mutation(prob));
        const auto dist(distance(i1, ind));

        CHECK(mut >= dist);

        if (i1.signature() != ind.signature())
        {
          CHECK(mut > 0);
          CHECK(dist > 0);
        }

        total_mut += mut;
        total_length += active_slots(i1);
      }

      const double perc(100.0 * total_mut / total_length);
      CHECK(perc > prob.env.evolution.p_mutation * 100.0 - 2.0);
      CHECK(perc < prob.env.evolution.p_mutation * 100.0 + 2.0);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Crossover")
{
  using namespace ultra;

  prob.env.slp.code_length = 100;

  unsigned different(0);
  const unsigned n(2000);
  for (unsigned j(0); j < n; ++j)
  {
    gp::individual i1(prob), i2(prob);

    i1.inc_age(random::sup(n));
    i2.inc_age(random::sup(n));

    const auto ic(crossover(i1, i2));
    CHECK(ic.is_valid());
    CHECK(ic.age() == std::max(i1.age(), i2.age()));

    if (ic != i1 && ic != i2)
      ++different;

    for (locus::index_t i(0); i < ic.size(); ++i)
      for (symbol::category_t c(0); c < ic.categories(); ++c)
      {
        const locus l{i, c};
        CHECK((ic[l] == i1[l] || ic[l] == i2[l]));
      }
  }

  CHECK(95 * n / 100 < different);
  CHECK(different < n);
}

TEST_CASE_FIXTURE(fixture3, "Random locus")
{
  using namespace ultra;

  for (unsigned k(0); k < 100; ++k)
  {
    std::map<locus, unsigned> exons;

    const gp::individual prg(prob);
    CHECK(prg.is_valid());

    const auto as(active_slots(prg));

    const unsigned n(10000);
    for (unsigned j(0); j < n; ++j)
      ++exons[random_locus(prg)];

    const double avg(n / as);
    for (const auto &e : exons)
    {
      CHECK(avg * 0.94 <= e.second);
      CHECK(e.second <= 1.06 * avg);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  for (unsigned i(0); i < 2000; ++i)
  {
    std::stringstream ss;
    gp::individual i1(prob);

    for (auto j(random::sup(10)); j; --j)
      i1.inc_age();

    CHECK(i1.save(ss));

    gp::individual i2(prob);
    CHECK(i2.load(ss, prob.sset));
    CHECK(i2.is_valid());

    CHECK(i1 == i2);
  }
}

TEST_CASE_FIXTURE(fixture1, "Output")
{
  using namespace ultra;

  gp::individual i(
    {
      {f_add, {2.0, z}},         // [0] ADD 2.0 Z()
      {f_add, {3.0, 4.0}},       // [1] ADD 3.0 4.0
      {f_sub, {0_addr, 1_addr}}  // [2] SUB [0] [1]
    });

  std::stringstream ss;

  SUBCASE("Dump")
  {
    ss << ultra::out::dump << i;
    CHECK(ss.str() == "[0] FADD 2 Z()\n"
                      "[1] FADD 3 4\n"
                      "[2] FSUB [0] [1]\n");
  }

  SUBCASE("Inline")
  {
    ss << ultra::out::in_line << i;
    CHECK(ss.str() == "FSUB FADD 2 Z() FADD 3 4");
  }

  SUBCASE("Graphviz")
  {
    ss << ultra::out::graphviz << i;
    CHECK(ss.str()
          == "graph\n"
             "{\n"
             "g2_0 [label=\"FSUB\", shape=box];\n"
             "g2_0 -- g0_0 [label=0, fontcolor=lightgray];\n"
             "g2_0 -- g1_0 [label=1, fontcolor=lightgray];\n"
             "g1_0 [label=\"FADD\", shape=box];\n"
             "g1_0 -- a1_0_0 [label=0, fontcolor=lightgray];\n"
             "a1_0_0 [label=3];\n"
             "g1_0 -- a1_0_1 [label=1, fontcolor=lightgray];\n"
             "a1_0_1 [label=4];\n"
             "g0_0 [label=\"FADD\", shape=box];\n"
             "g0_0 -- a0_0_0 [label=0, fontcolor=lightgray];\n"
             "a0_0_0 [label=2];\n"
             "g0_0 -- a0_0_1 [label=1, fontcolor=lightgray];\n"
             "a0_0_1 [label=\"Z()\"];\n"
             "}");
  }

  SUBCASE("List")
  {
    ss << ultra::out::list << i;
    CHECK(ss.str() == "[2] FSUB [0] [1]\n"
                      "[1] FADD 3 4\n"
                      "[0] FADD 2 Z()\n");
  }

  SUBCASE("Tree")
  {
    ss << ultra::out::tree << i;
    CHECK(ss.str() == "FSUB\n"
                      "  FADD\n"
                      "    2\n"
                      "    Z()\n"
                      "  FADD\n"
                      "    3\n"
                      "    4\n");
  }
}

TEST_CASE_FIXTURE(fixture3, "Output full multicategories")
{
  using namespace ultra;

  gp::individual i(
    {
      {s_ife, {s1->instance(), s2->instance(), s1->instance(), s3->instance()}},
                                  // [0] SIFE "hello" "world" "hello" ":-)"
      {f_len, {0_addr}},          // [1] FLENGTH [0]
      {f_len, {s2->instance()}},  // [2] FLENGTH "world"
      {f_add, {1_addr, 2_addr}}   // [3] FADD [1] [2]
    });

  std::stringstream ss;

  SUBCASE("Dump")
  {
    ss << ultra::out::dump << i;
    CHECK(ss.str() == "[0,0]\n"
                      "[0,1] SIFE \"hello\" \"world\" \"hello\" \":-)\"\n"
                      "[1,0] FLENGTH [0,1]\n"
                      "[1,1]\n"
                      "[2,0] FLENGTH \"world\"\n"
                      "[2,1]\n"
                      "[3,0] FADD [1,0] [2,0]\n"
                      "[3,1]\n");
  }

  SUBCASE("Inline")
  {
    ss << ultra::out::in_line << i;
    CHECK(ss.str()
          == "FADD FLENGTH SIFE \"hello\" \"world\" \"hello\" \":-)\" FLENGTH \"world\"");
  }

  SUBCASE("Graphviz")
  {
    ss << ultra::out::graphviz << i;
    CHECK(ss.str()
          == "graph\n"
             "{\n"
             "g3_0 [label=\"FADD\", shape=box];\n"
             "g3_0 -- g1_0 [label=0, fontcolor=lightgray];\n"
             "g3_0 -- g2_0 [label=1, fontcolor=lightgray];\n"
             "g2_0 [label=\"FLENGTH\", shape=box];\n"
             "g2_0 -- a2_0_0 [label=0, fontcolor=lightgray];\n"
             "a2_0_0 [label=\"world\"];\n"
             "g1_0 [label=\"FLENGTH\", shape=box];\n"
             "g1_0 -- g0_1 [label=0, fontcolor=lightgray];\n"
             "g0_1 [label=\"SIFE\", shape=box];\n"
             "g0_1 -- a0_1_0 [label=0, fontcolor=lightgray];\n"
             "a0_1_0 [label=\"hello\"];\n"
             "g0_1 -- a0_1_1 [label=1, fontcolor=lightgray];\n"
             "a0_1_1 [label=\"world\"];\n"
             "g0_1 -- a0_1_2 [label=2, fontcolor=lightgray];\n"
             "a0_1_2 [label=\"hello\"];\n"
             "g0_1 -- a0_1_3 [label=3, fontcolor=lightgray];\n"
             "a0_1_3 [label=\":-)\"];\n"
             "}");
  }

  SUBCASE("List")
  {
    ss << ultra::out::list << i;
    CHECK(ss.str() == "[3,0] FADD [1,0] [2,0]\n"
                      "[2,0] FLENGTH \"world\"\n"
                      "[1,0] FLENGTH [0,1]\n"
                      "[0,1] SIFE \"hello\" \"world\" \"hello\" \":-)\"\n");
  }

  SUBCASE("Tree")
  {
    ss << ultra::out::tree << i;
    CHECK(ss.str() == "FADD\n"
                      "  FLENGTH\n"
                      "    SIFE\n"
                      "      \"hello\"\n"
                      "      \"world\"\n"
                      "      \"hello\"\n"
                      "      \":-)\"\n"
                      "  FLENGTH\n"
                      "    \"world\"\n");
  }
}

}  // TEST_SUITE("GP INDIVIDUAL")
