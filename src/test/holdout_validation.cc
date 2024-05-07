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

const std::string iris(R"(
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

TEST_CASE("Concepts")
{
  using namespace ultra;

  CHECK(ValidationStrategy<src::holdout_validation>);
}

TEST_CASE("Cardinality")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(iris);
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

  std::istringstream is(iris);
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
