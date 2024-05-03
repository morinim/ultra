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

#include <numbers>

#include "kernel/symbol_set.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/primitive/string.h"
#include "utility/misc.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("SYMBOL SET")
{

TEST_CASE("Constructor / Insertion")
{
  using namespace ultra;

  symbol_set ss;

  SUBCASE("Empty symbol set")
  {
    CHECK(!ss.categories());
    CHECK(!ss.terminals());
    CHECK(ss.enough_terminals());
    CHECK(ss.categories_missing_terminal().empty());
    CHECK(ss.is_valid());
  }

  SUBCASE("Single category symbol set")
  {
    ss.insert<real::sin>();
    ss.insert<real::cos>();
    ss.insert<real::add>();
    ss.insert<real::sub>();
    ss.insert<real::div>();
    ss.insert<real::mul>();
    CHECK(ss.categories() == 1);
    CHECK(!ss.terminals());
    CHECK(!ss.enough_terminals());
    CHECK(ss.categories_missing_terminal()
          == std::set{symbol::default_category});

    ss.insert<real::number>();
    CHECK(ss.categories() == 1);
    CHECK(ss.terminals() == 1);
    CHECK(ss.enough_terminals());
    CHECK(ss.categories_missing_terminal().empty());
    CHECK(ss.is_valid());

    // Reset
    ss.clear();
    CHECK(ss.categories() == 0);
    CHECK(ss.enough_terminals());
    CHECK(ss.categories_missing_terminal().empty());
    CHECK(ss.is_valid());
  }

  SUBCASE("Multi-category symbol set")
  {
    ss.insert<real::add>();
    ss.insert<real::number>();
    ss.insert<str::ife>(0, function::param_data_types{1, 1, 0, 0});
    CHECK(ss.categories() == 1);
    CHECK(ss.terminals() == 1);
    CHECK(!ss.enough_terminals());
    CHECK(ss.categories_missing_terminal() == std::set<symbol::category_t>{1});

    ss.insert<str::literal>("apple", 1);
    CHECK(ss.categories() == 2);
    CHECK(ss.terminals(0) == 1);
    CHECK(ss.terminals(1) == 1);
    CHECK(ss.enough_terminals());
    CHECK(ss.categories_missing_terminal().empty());
    CHECK(ss.is_valid());

    CHECK(ss.decode("apple"));
    const auto *s(get_if<str::literal>(ss.decode("apple")));
    CHECK(s);
    CHECK(std::get<std::string>(s->instance()) == "apple");
    CHECK(ss.decode("FADD"));
    CHECK(ss.decode("SIFE"));
    CHECK(ss.decode("REAL"));
  }
}

TEST_CASE("Distribution")
{
  using namespace ultra;

  symbol_set ss;

  // Initial setup
  const symbol_set::weight_t number_weight(400);
  const symbol_set::weight_t apple_weight(300), orange_weight(100);

  const terminal *number, *apple, *orange;

  const std::vector<const symbol *> symbols[2] =
  {
    {
      number = ss.insert<real::number, number_weight>(),
      ss.insert<real::add, 300>(),
      ss.insert<real::sub, 200>(),
      ss.insert<str::ife, 200>(0, function::param_data_types{1, 1, 0, 0}),
      ss.insert<real::mul, 100>()
    },
    {
      apple = ss.insert<str::literal, apple_weight>("apple", 1),
      orange = ss.insert<str::literal, orange_weight>("orange", 1),
      ss.insert<str::ife, 200>(1, function::param_data_types{1, 1, 1, 1}),
    }
  };

  const std::map<const symbol *, symbol_set::weight_t> wanted =
  {
    {symbols[0][0], number_weight},
    {symbols[0][1], 300},
    {symbols[0][2], 200},
    {symbols[0][3], 200},
    {symbols[0][4], 100},
    {symbols[1][0], apple_weight},
    {symbols[1][1], orange_weight},
    {symbols[1][2], 200}
  };

  const symbol_set::weight_t sum_c[2] =
  {
    number_weight,
    apple_weight + orange_weight
  };

  const auto ratio = [&symbols](const auto &container, const symbol *sym)
  {
    const auto val(container.at(sym));

    const symbol *ref(is<terminal>(sym) ? symbols[sym->category()][0]
                                        : symbols[sym->category()].back());
    assert(ref->category() == sym->category());
    assert(is<terminal>(ref) == is<terminal>(sym));

    const auto ref_val(container.at(ref));
    assert(ref_val > 0);

    return static_cast<double>(val) / static_cast<double>(ref_val);
  };

  for (symbol::category_t c(0); c < ss.categories(); ++c)
    for (const auto *s : symbols[c])
      CHECK(ss.weight(*s) == wanted.at(s));

  constexpr unsigned n(500000);
  constexpr double eps(0.02);
  std::map<const symbol *, unsigned> hist;

  SUBCASE("roulette_function")
  {
    for (unsigned i(0); i < n; ++i)
      ++hist[ss.roulette_function()];

    for (const auto *s : symbols[0])
      if (!is<terminal>(s))
      {
        CHECK(hist[s] > 0);
        CHECK(ratio(hist, s)
              == doctest::Approx(ratio(wanted, s)).epsilon(eps));
      }
  }

  SUBCASE("roulette_terminal")
  {
    for (unsigned i(0); i < n; ++i)
    {
      const symbol::category_t c(random::boolean());
      if (c == 0)
      {
        CHECK(ss.roulette_terminal(c).index() == d_double);
        ++hist[number];
      }
      else
      {
        const auto v(ss.roulette_terminal(c));
        CHECK(v.index() == d_string);
        ++hist[v == apple->instance() ? apple : orange];
      }
    }

    for (symbol::category_t c(0); c < ss.categories(); ++c)
      for (const auto *s : symbols[c])
        if (is<terminal>(s))
        {
          CHECK(hist[s] > 0);
          CHECK(ratio(hist, s)
                == doctest::Approx(ratio(wanted, s)).epsilon(eps));
        }
  }

  SUBCASE("roulette_terminal with parameters")
  {
    const std::size_t sup(11);
    const symbol_set::weight_t weight(100);

    unsigned count_p[2] = {0, 0};
    for (unsigned i(0); i < n; ++i)
    {
      const symbol::category_t c(random::boolean());
      if (const auto v(ss.roulette_terminal(sup, c, weight));
          v.index() == d_address)
        ++count_p[c];
    }

    for (symbol::category_t c(0); c <= 1; ++c)
    {
      const auto expected(weight * n / (sum_c[c] + weight) / 2);
      const auto actual(count_p[c]);

      CHECK(98 * expected <= 100 * actual);
      CHECK(100 * actual <= 102 *expected);
    }
  }

  SUBCASE("roulette")
  {
    for (unsigned i(0); i < n; ++i)
      ++hist[ss.roulette()];

    const auto sum_f(std::accumulate(hist.begin(), hist.end(), 0,
                                     [](auto sum, auto e)
                                     {
                                       return is<terminal>(e.first) ?
                                              sum : sum + e.second;
                                     }));
    const auto sum_t(std::accumulate(hist.begin(), hist.end(), 0,
                                     [](auto sum, auto e)
                                     {
                                       return is<terminal>(e.first) ?
                                              sum + e.second : sum;
                                     }));
    CHECK(std::max(sum_f, sum_t) - std::min(sum_f, sum_t) < n / 100);

    for (const auto *s : symbols[0])
    {
      CHECK(hist[s] > 0);

      if (is<terminal>(s))
        CHECK(doctest::Approx(ratio(hist, s)).epsilon(eps)
              == ratio(wanted, s) * sum_f / sum_t);
    }
  }

  SUBCASE("roulette_free")
  {
    for (unsigned i(0); i < n; ++i)
      ++hist[ss.roulette_free(random::boolean())];

    for (symbol::category_t c(0); c < ss.categories(); ++c)
      for (const auto *s : symbols[c])
      {
        CHECK(hist[s] > 0);
        CHECK(ratio(hist, s)
              == doctest::Approx(ratio(wanted, s)).epsilon(eps));
      }
  }
}

}  // TEST_SUITE("FUNCTION")
