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
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "kernel/cache.h"
#include "kernel/parameters.h"
#include "kernel/gp/interpreter.h"

#include "test/fixture1.h"


TEST_SUITE("CACHE")
{

TEST_CASE_FIXTURE(fixture1, "Constructor")
{
  using namespace ultra;

  cache<double> cache;
  CHECK(!cache.bits());

  for (unsigned i(1); i < 8; ++i)
  {
    cache.resize(i);
    CHECK(cache.bits() == i);
  }
}

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

    const auto sf(cache.find(i1.signature()));
    CHECK(sf);
    CHECK(almost_equal(*sf, f));
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
    if (const auto f = cache.find(vi[i].signature()))
    {
      const auto val(run(vi[i]));

      const auto f1{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};
      CHECK(almost_equal(*f, f1));
    }
}

TEST_CASE_FIXTURE(fixture1, "Concurrent access")
{
  using namespace ultra;

  constexpr std::size_t sup(1000);
  constexpr unsigned cycles(100);

  // Automatically scales to system capabilities while ensuring at least one
  // thread for reads and writes.
  const auto n_threads(
    std::max<unsigned>(std::thread::hardware_concurrency(), 2));
  const auto r_threads(n_threads / 2);
  const auto w_threads(n_threads - r_threads);

  std::vector<std::uint64_t> reads(r_threads, 0), writes(w_threads, 0);

  cache<double> cache(14);

  std::vector<std::pair<hash_t, double>> ind_db;
  ind_db.reserve(sup);

  for (std::size_t i(0); i < sup; ++i)
  {
    const hash_t sig(i, i);
    const auto fit(static_cast<double>(i));
    ind_db.emplace_back(sig, fit);

    // Cache warm-up.
    cache.insert(sig, fit);
  }

  const auto writer([&](unsigned idx)
  {
    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
      {
        cache.insert(ind_db[i].first, ind_db[i].second);
        ++writes[idx];
      }
  });

  const auto reader([&](unsigned idx)
  {
    for (unsigned j(0); j < cycles; ++j)
      for (unsigned i(0); i < sup; ++i)
        if (const auto fit(cache.find(ind_db[i].first)); fit)
        {
          ++reads[idx];

          CHECK(almost_equal(*fit, ind_db[i].second));
        }
  });

  std::vector<std::thread> threads;

  for (unsigned i(0); i < std::max(r_threads, w_threads); ++i)
  {
    // Alternating between reader and writer thread creation could smooth
    // resource usage.
    if (i < r_threads)
      threads.emplace_back(reader, i);
    if (i < w_threads)
      threads.emplace_back(writer, i);
  }

  for (auto &thread: threads)
    thread.join();

  for (auto w : writes)
    CHECK(w == sup * cycles);

  const auto base(reads.front());
  for (auto r : reads)
  {
    const auto min(base - base * 0.05);
    const auto max(base + base * 0.05);
    const bool in_range(min <= r && r <= max);
    CHECK(in_range);

    CHECK(r <= sup * cycles);
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
                           return cache1.find(i.signature()).has_value();
                         });

  std::stringstream ss;
  CHECK(cache1.save(ss));

  CHECK(cache2.load(ss));
  for (unsigned i(0); i < n; ++i)
    if (present[i])
    {
      const auto val(run(vi[i]));
      const auto f1{has_value(val) ? std::get<D_DOUBLE>(val) : 0.0};

      const auto f(cache2.find(vi[i].signature()));
      CHECK(f);
      CHECK(almost_equal(*f, f1));
    }
}

}  // TEST_SUITE("CACHE")
