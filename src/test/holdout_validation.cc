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

#include "kernel/gp/src/holdout_validation.h"
#include "kernel/gp/src/problem.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

bool near_integers(std::size_t x, std::size_t y)
{
  return x == y
         || (x + 1 == y && x + 1 != 0)
         || (y + 1 == x && y + 1 != 0);
}

TEST_SUITE("HOLDOUT VALIDATION")
{

TEST_CASE("Concepts")
{
  using namespace ultra;

  CHECK(ValidationStrategy<src::holdout_validation>);
}

TEST_CASE("Cardinality")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::iris_full);
  src::problem prob(is);
  CHECK(!!prob);

  const auto orig(prob.data());
  const auto examples(prob.data().size());

  for (unsigned perc_train(1); perc_train < 100; ++perc_train)
    for (unsigned perc_val(0); perc_val + perc_train <= 100; ++perc_val)
    {
      src::holdout_validation v(prob, perc_train, perc_val);

      CHECK(near_integers(prob.data(src::dataset_t::training).size(),
                          examples * perc_train / 100));

      CHECK(near_integers(prob.data(src::dataset_t::validation).size(),
                          examples * perc_val / 100));

      CHECK(near_integers(prob.data(src::dataset_t::test).size(),
                          examples * (100 - perc_train - perc_val) / 100));

      CHECK(prob.data(src::dataset_t::training).size()
            + prob.data(src::dataset_t::validation).size()
            + prob.data(src::dataset_t::test).size() == examples);

      prob.data(src::dataset_t::training) = orig;
      prob.data(src::dataset_t::validation).clear();
      prob.data(src::dataset_t::test).clear();
    }
}

TEST_CASE("Probabilities")
{
  using namespace ultra;

  std::istringstream is(debug::iris_full);
  src::problem prob(is);
  CHECK(!!prob);

  // Output value changed to be used as unique key for example identification.
  int i(0);
  for (auto &e : prob.data())
    e.output = i++;

  const auto orig(prob.data());
  const auto examples(orig.size());

  std::vector<unsigned> count(examples);

  const std::size_t extractions(10000);
  const unsigned validation_perc(30);

  for (unsigned j(0); j < extractions; ++j)
  {
    src::holdout_validation v(prob, 40, validation_perc);

    for (const auto &e : prob.data(src::dataset_t::validation))
      ++count[std::get<int>(e.output)];

    prob.data(src::dataset_t::training) = orig;
    prob.data(src::dataset_t::validation).clear();
    prob.data(src::dataset_t::test).clear();
  }

  const auto expected(extractions * validation_perc / 100);
  const auto tolerance_perc(10);
  const auto tolerance_inf(expected * (100 - tolerance_perc) / 100);
  const auto tolerance_sup(expected * (100 + tolerance_perc) / 100);

  for (auto x : count)
  {
    CHECK(x > tolerance_inf);
    CHECK(x < tolerance_sup);
  }
}

}  // TEST_SUITE("HOLDOUT VALIDATION")
