/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "kernel/gp/src/predictor.h"

#include "kernel/problem.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace
{

using ultra::value_t;
using ultra::src::classification_result;

struct reg_ok
{
  value_t operator()(const std::vector<value_t> &) const;
};

struct reg_bad
{
  int operator()(const std::vector<value_t> &) const;
};

struct class_ok
{
  classification_result tag(const std::vector<value_t> &) const;
};

struct class_bad
{
  std::size_t tag(const std::vector<value_t> &) const;
};

struct rich_class_ok
{
  value_t operator()(const std::vector<value_t> &) const;
  classification_result tag(const std::vector<value_t> &) const;
};

}  // namespace

TEST_SUITE("predictor concepts")
{

TEST_CASE("dummy")
{
  static_assert(ultra::src::RegressionPredictor<reg_ok>);
  static_assert(!ultra::src::RegressionPredictor<reg_bad>);

  static_assert(ultra::src::ClassificationPredictor<class_ok>);
  static_assert(!ultra::src::ClassificationPredictor<class_bad>);

  static_assert(ultra::src::ClassificationPredictor<rich_class_ok>);
  static_assert(ultra::src::RichClassificationPredictor<rich_class_ok>);
  static_assert(!ultra::src::RichClassificationPredictor<class_ok>);

  CHECK(true);
}

}  // TEST_SUITE
