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

TEST_CASE_FIXTURE(fixture1, "Construction from vector")
{
  using namespace ultra;

  gp::individual i({
                     {f_add, {3.0, 2.0}},       // [0] ADD $3.0, $2.0
                     {f_add, {0_addr, 1.0}},    // [1] ADD [0], $1.0
                     {f_sub, {1_addr, 0_addr}}  // [2] SUB [1], [0]
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
      {f_add, {2.0, z}},          // [0] ADD 2.0 Z()
      {f_add, {3.0, 4.0}},        // [1] ADD 3.0 4.0
      {f_sub, {0_addr, 1_addr}},  // [2] SUB [0] [1]
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
}

}  // TEST_SUITE("GP INDIVIDUAL")
