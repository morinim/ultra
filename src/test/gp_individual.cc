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

}  // TEST_SUITE("GP INDIVIDUAL")
