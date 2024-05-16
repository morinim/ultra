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

  const auto orig(prob.data.selected());
  const auto examples(prob.data.selected().size());

  for (unsigned perc_train(1); perc_train < 100; ++perc_train)
    for (unsigned perc_val(0); perc_val + perc_train <= 100; ++perc_val)
    {
      src::holdout_validation::params params;
      params.training_perc = perc_train;
      params.validation_perc = perc_val;
      params.stratify = false;

      src::holdout_validation v(prob, params);

      CHECK(near_integers(prob.data[src::dataset_t::training].size(),
                          examples * perc_train / 100));

      CHECK(near_integers(prob.data[src::dataset_t::validation].size(),
                          examples * perc_val / 100));

      CHECK(near_integers(prob.data[src::dataset_t::test].size(),
                          examples * (100 - perc_train - perc_val) / 100));

      CHECK(prob.data[src::dataset_t::training].size()
            + prob.data[src::dataset_t::validation].size()
            + prob.data[src::dataset_t::test].size() == examples);

      prob.data[src::dataset_t::training] = orig;
      prob.data[src::dataset_t::validation].clear();
      prob.data[src::dataset_t::test].clear();
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
  for (auto &e : prob.data.selected())
    e.output = i++;

  const auto orig(prob.data.selected());
  const auto examples(orig.size());

  std::vector<unsigned> count(examples);

  const std::size_t extractions(10000);
  const unsigned validation_perc(30);

  src::holdout_validation::params params;
  params.training_perc = 40;
  params.validation_perc = validation_perc;
  params.stratify = false;

  for (unsigned j(0); j < extractions; ++j)
  {
    src::holdout_validation v(prob, params);

    for (const auto &e : prob.data[src::dataset_t::validation])
      ++count[std::get<int>(e.output)];

    prob.data[src::dataset_t::training] = orig;
    prob.data[src::dataset_t::validation].clear();
    prob.data[src::dataset_t::test].clear();
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

TEST_CASE("Stratify")
{
  using namespace ultra;

  std::istringstream is(debug::iris_full);
  src::problem prob(is);
  CHECK(!!prob);

  const auto orig(prob.data.selected());
  const auto examples(orig.size());

  src::holdout_validation::params params;
  params.training_perc = 60;
  params.validation_perc = 20;
  params.stratify = true;

  for (unsigned cycles(10); cycles; --cycles)
  {
    src::holdout_validation v(prob, params);

    CHECK(near_integers(prob.data[src::dataset_t::training].size(),
                        examples * params.training_perc / 100));
    CHECK(near_integers(prob.data[src::dataset_t::validation].size(),
                        examples * params.validation_perc / 100));
    CHECK(near_integers(prob.data[src::dataset_t::test].size(),
                        examples
                        * (100 - params.training_perc - params.validation_perc)
                        / 100));

    const std::vector indices = {src::dataset_t::training,
                                 src::dataset_t::validation,
                                 src::dataset_t::test};
    std::vector<std::map<value_t, double>> count(indices.size());

    for (auto index : indices)
      for (auto example : prob.data[index])
        ++count[as_integer(index)][example.output];

    for (auto pair : count[0])
    {
      const double ref_perc(pair.second
                            / prob.data[src::dataset_t::training].size());

      const double perc_v(count[1][pair.first]
                          / prob.data[src::dataset_t::validation].size());

      CHECK(perc_v == doctest::Approx(ref_perc));

      const double perc_t(count[2][pair.first]
                          / prob.data[src::dataset_t::test].size());

      CHECK(perc_t == doctest::Approx(ref_perc));
    }

    prob.data[src::dataset_t::training] = orig;
    prob.data[src::dataset_t::validation].clear();
    prob.data[src::dataset_t::test].clear();
  }
}

}  // TEST_SUITE("HOLDOUT VALIDATION")
