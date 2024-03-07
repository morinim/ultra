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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "kernel/cache.h"
#include "kernel/parameters.h"
#include "kernel/gp/interpreter.h"

#include "test/fixture1.h"


[[nodiscard]] bool approx_equal(double v1, double v2)
{
  if (std::isinf(v1) && std::isinf(v2) && std::signbit(v1) == std::signbit(v2))
    return true;

  return v1 == doctest::Approx(v2);
}

TEST_SUITE("CACHE")
{

TEST_CASE_FIXTURE(fixture1, "Insert/Find cycle")
{
  using namespace ultra;

  cache<double> cache(16);
  prob.params.slp.code_length = 64;

  const unsigned n(6000);

  for (unsigned i(0); i < n; ++i)
  {
    const ultra::gp::individual i1(prob);
    const auto f(static_cast<double>(i));

    cache.insert(i1.signature(), f);

    const auto *sf(cache.find(i1.signature()));
    CHECK(sf);
    CHECK(approx_equal(*sf, f));
  }
}

TEST_CASE_FIXTURE(fixture1, "Collision detection")
{
  using namespace ultra;

  cache<double> cache(14);
  prob.params.slp.code_length = 64;

  const unsigned n(1000);

  std::vector<gp::individual> vi;
  for (unsigned i(0); i < n; ++i)
  {
    gp::individual i1(prob);
    const auto val(run(i1));
    auto f{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};

    cache.insert(i1.signature(), f);
    vi.push_back(i1);
  }

  for (unsigned i(0); i < n; ++i)
    if (const auto *f = cache.find(vi[i].signature()))
    {
      const auto val(run(vi[i]));

      const auto f1{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};
      CHECK(approx_equal(*f, f1));
    }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  cache<double> cache1(14), cache2(14);
  prob.params.slp.code_length = 64;

  const unsigned n(1000);
  std::vector<gp::individual> vi;
  for (unsigned i(0); i < n; ++i)
  {
    gp::individual i1(prob);
    const auto val(run(i1));
    const auto f{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};

    cache1.insert(i1.signature(), f);
    vi.push_back(i1);
  }

  std::vector<std::uint8_t> present(n);
  std::ranges::transform(vi, present.begin(),
                         [&cache1](const auto &i)
                         {
                           return cache1.find(i.signature()) != nullptr;
                         });

  std::stringstream ss;
  CHECK(cache1.save(ss));

  CHECK(cache2.load(ss));
  for (unsigned i(0); i < n; ++i)
    if (present[i])
    {
      const auto val(run(vi[i]));
      const auto f1{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};

      const auto *f(cache2.find(vi[i].signature()));
      CHECK(f);
      CHECK(approx_equal(*f, f1));
    }
}

}  // TEST_SUITE("CACHE")
