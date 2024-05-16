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
#include <iostream>

#include "kernel/gp/src/multi_dataset.h"
#include "kernel/gp/src/dataframe.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("MULTI DATASET")
{

TEST_CASE("Concepts")
{
  CHECK(ultra::src::DataSet<ultra::src::dataframe>);
}

TEST_CASE("Base")
{
  using namespace ultra::src;

  multi_dataset<dataframe> mds;

  CHECK(mds[dataset_t::training].empty());
  CHECK(mds[dataset_t::validation].empty());
  CHECK(mds[dataset_t::test].empty());

  CHECK(mds.selected().empty());

  const std::size_t nr(1000);
  for (std::size_t i(0); i < nr; ++i)
  {
    example ex;

    ex.input = std::vector<ultra::value_t>{ultra::random::sup(1000.0),
                                           ultra::random::sup(1000.0),
                                           ultra::random::sup(1000.0)};
    ex.output = ultra::random::sup(1000.0);

    mds.selected().push_back(ex);
  }

  CHECK(mds.selected().size() == nr);

  CHECK(mds[dataset_t::training].size() == nr);
  CHECK(mds[dataset_t::validation].empty());
  CHECK(mds[dataset_t::test].empty());

  mds.select(dataset_t::validation);
  mds.selected().push_back(mds[dataset_t::training].front());

  CHECK(mds.selected().size() == 1);

  CHECK(mds[dataset_t::training].size() == nr);
  CHECK(mds[dataset_t::validation].size() == 1);
  CHECK(mds[dataset_t::test].empty());

  mds.select(dataset_t::test);
  mds.selected().push_back(mds[dataset_t::training].front());
  mds.selected().push_back(mds[dataset_t::training].front());

  CHECK(mds.selected().size() == 2);

  CHECK(mds[dataset_t::training].size() == nr);
  CHECK(mds[dataset_t::validation].size() == 1);
  CHECK(mds[dataset_t::test].size() == 2);
}

}  // TEST_SUITE("MULTI DATASET")
