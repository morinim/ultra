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

#include "kernel/gp/individual.h"
#include "kernel/gp/team.h"
#include "kernel/gp/primitive/integer.h"
#include "kernel/gp/primitive/real.h"
#include "kernel/gp/src/oracle.h"
#include "kernel/gp/src/problem.h"
#include "kernel/gp/src/variable.h"

#include <cstdlib>
#include <future>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#define TEST_WTA

template<template<class> class L, ultra::Individual T, unsigned P>
struct build
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d, P);
  }
};

template<template<class> class L, ultra::Individual T>
struct build<L, T, 0>
{
  L<T> operator()(const T &prg, ultra::src::dataframe &d) const
  {
    return L<T>(prg, d);
  }
};

template<ultra::Individual T>
struct build<ultra::src::reg_oracle, T, 0>
{
  ultra::src::reg_oracle<T> operator()(const T &prg,
                                       ultra::src::dataframe &) const
  {
    return ultra::src::reg_oracle(prg);
  }
};

template<template<class> class L, ultra::Individual T, unsigned P = 0>
void test_serialization(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned cycles(256); cycles; --cycles)
  {
    const T ind(pr);
    const auto oracle1(build<L, T, P>()(ind, pr.data.selected()));

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load<T>(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1.name(oracle1(e.input)));
      const auto out2(oracle2->name((*oracle2)(e.input)));

      CHECK(out1 == out2);
    }
  }
}

template<template<class> class L, unsigned P = 0>
void test_team_of_one(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned i(0); i < 1000; ++i)
  {
    const gp::individual ind(pr);
    const auto li(build<L, gp::individual, P>()(ind, pr.data.selected()));

    const gp::team<gp::individual> t{{ind}};
    const auto lt(build<L, gp::team<gp::individual>, P>()(t,
                                                          pr.data.selected()));

    for (const auto &e : pr.data.selected())
    {
      const auto out_i(li(e.input)), out_t(lt(e.input));

      if (has_value(out_i))
      {
        const auto v1(lexical_cast<D_DOUBLE>(out_i));
        const auto v2(lexical_cast<D_DOUBLE>(out_t));

        CHECK(almost_equal(v1, v2));
      }
      else
        CHECK(!has_value(out_t));
    }
  }
}

template<template<class> class L, unsigned P = 0>
void test_team(ultra::src::problem &pr)
{
  using namespace ultra;

  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual ind1(pr);
    const gp::individual ind2(pr);
    const gp::individual ind3(pr);

    const auto oracle1(build<L, gp::individual, P>()(ind1, pr.data.selected()));
    const auto oracle2(build<L, gp::individual, P>()(ind2, pr.data.selected()));
    const auto oracle3(build<L, gp::individual, P>()(ind3, pr.data.selected()));

    const gp::team<gp::individual> t{{ind1, ind2, ind3}};
    const auto ts(t.size());
    const auto oracle_t(build<L, gp::team<gp::individual>, P>()(
                          t, pr.data.selected()));

    for (const auto &example : pr.data.selected())
    {
      const std::vector out =
      {
        oracle1(example.input), oracle2(example.input), oracle3(example.input)
      };
      const std::vector<std::string> names =
      {
        oracle1.name(out[0]), oracle2.name(out[1]), oracle3.name(out[2])
      };
      const std::vector<src::classification_result> tags =
      {
        oracle1.tag(example.input), oracle2.tag(example.input),
        oracle3.tag(example.input)
      };

      for (std::size_t j(0); j < ts; ++j)
        CHECK(std::get<D_INT>(out[j]) == tags[j].label);

      std::string s_best(names[0]);

#if defined(TEST_MV)
      std::map<std::string, unsigned> votes;

      for (std::size_t j(0); j < ts; ++j)
      {
        if (votes.find(names[j]) == votes.end())
          votes[names[j]] = 1;
        else
          ++votes[names[j]];
      }

      unsigned v_best(0);

      for (auto &v : votes)
        if (v.second > v_best)
        {
          s_best = v.first;
          v_best = v.second;
        }
#elif defined(TEST_WTA)
      src::class_t c_best(0);

      for (std::size_t j(1); j < ts; ++j)
        if (tags[j].sureness > tags[c_best].sureness)
        {
          s_best = names[j];
          c_best = j;
        }
#endif

      CHECK(s_best == oracle_t.name(oracle_t(example.input)));
    }
  }
}

struct fixture
{
  fixture() { pr.params.init(); }

  ultra::src::problem pr {};
};

TEST_SUITE("ORACLE")
{

TEST_CASE_FIXTURE(fixture, "reg_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::sr);
  CHECK(pr.data.selected().read(is) == debug::SR_COUNT);
  pr.setup_symbols();

  // TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::reg_oracle>(pr);

  // TEAM OF IDENTICAL INDIVIDUALS.
  for (unsigned i(0); i < 1000; ++i)
  {
    const gp::individual ind(pr);
    const src::reg_oracle li(ind);

    const gp::team<gp::individual> t{{ind, ind, ind, ind}};
    const src::reg_oracle lt(t);

    for (const auto &e : pr.data.selected())
    {
      const auto out_i(li(e.input)), out_t(lt(e.input));

      if (has_value(out_i))
      {
        const auto v1(std::get<D_DOUBLE>(out_i));
        const auto v2(std::get<D_DOUBLE>(out_t));

        CHECK(almost_equal(v1, v2));
      }
      else
        CHECK(!has_value(out_t));
    }
  }

  // TEAM OF RANDOM INDIVIDUALS.
  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual i1(pr);
    const gp::individual i2(pr);
    const gp::individual i3(pr);
    const gp::individual i4(pr);

    const src::reg_oracle oracle1(i1);
    const src::reg_oracle oracle2(i2);
    const src::reg_oracle oracle3(i3);
    const src::reg_oracle oracle4(i4);

    const gp::team<gp::individual> t({i1, i2, i3, i4});
    const src::reg_oracle oracle_team(t);

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1(e.input));
      const auto out2(oracle2(e.input));
      const auto out3(oracle3(e.input));
      const auto out4(oracle4(e.input));

      D_DOUBLE sum(0.0), n(0.0);
      if (has_value(out1))
      {
        sum += lexical_cast<D_DOUBLE>(out1);
        ++n;
      }
      if (has_value(out2))
      {
        sum += lexical_cast<D_DOUBLE>(out2);
        ++n;
      }
      if (has_value(out3))
      {
        sum += lexical_cast<D_DOUBLE>(out3);
        ++n;
      }
      if (has_value(out4))
      {
        sum += lexical_cast<D_DOUBLE>(out4);
        ++n;
      }

      if (n > 0.0)
      {
        const auto out_t(oracle_team(e.input));

        if (!almost_equal(sum / n, std::get<D_DOUBLE>(out_t)))
        {
          ultraWARNING("{}  {}  {}  {}       {} {}\n",
                         std::get<D_DOUBLE>(out1),
                         std::get<D_DOUBLE>(out2),
                         std::get<D_DOUBLE>(out3),
                         std::get<D_DOUBLE>(out4), sum / n,
                         std::get<D_DOUBLE>(out_t));
        }

        CHECK(sum / n == doctest::Approx(std::get<D_DOUBLE>(out_t)));
      }
    }
  }
}


TEST_CASE_FIXTURE(fixture, "reg_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::sr);
  CHECK(pr.data.selected().read(is) == debug::SR_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  for (unsigned cycles(1000); cycles; --cycles)
  {
    const gp::individual ind(pr);
    const src::reg_oracle oracle1(ind);

    std::stringstream ss;

    CHECK(src::serialize::save(ss, oracle1));
    const auto oracle2(src::serialize::oracle::load(ss, pr.sset));
    REQUIRE(oracle2);
    REQUIRE(oracle2->is_valid());

    for (const auto &e : pr.data.selected())
    {
      const auto out1(oracle1(e.input));
      const auto out2((*oracle2)(e.input));

      if (has_value(out1))
        CHECK(std::get<D_DOUBLE>(out1)
              == doctest::Approx(std::get<D_DOUBLE>(out2)));
      else
        CHECK(!has_value(out2));
    }
  }
}

TEST_CASE_FIXTURE(fixture, "gaussian_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::iris_full);
  CHECK(pr.data.selected().read(is) == debug::IRIS_FULL_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // GAUSSIAN ORACLE TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::gaussian_oracle>(pr);

  // GAUSSIAN ORACLE TEAM OF RANDOM INDIVIDUALS.
  test_team<src::gaussian_oracle>(pr);
}

TEST_CASE_FIXTURE(fixture, "gaussian_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::iris_full);
  CHECK(pr.data.selected().read(is) == debug::IRIS_FULL_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // GAUSSIAN_ORACLE SERIALIZATION - INDIVIDUAL.
  test_serialization<src::gaussian_oracle, gp::individual>(pr);

  // GAUSSIAN_ORACLE SERIALIZATION - TEAM.
  test_serialization<src::gaussian_oracle, gp::team<gp::individual>>(pr);
}

TEST_CASE_FIXTURE(fixture, "gaussian_oracle missing inference fallback")
{
  using namespace ultra;

  std::istringstream is(R"(
    CLASS, X
    C0, 1.0
    C0, 2.0
    C0, 3.0
    C1, 4.0
  )");

  CHECK(pr.data.selected().read(is) == 4);
  pr.setup_symbols();

  const auto *var_x(static_cast<const src::variable *>(pr.sset.decode("X")));
  REQUIRE(var_x);

  const function *f_div(pr.insert<real::div>());
  const gp::individual ind({{f_div, {1.0, var_x}}});

  const src::gaussian_oracle oracle(ind, pr.data.selected());
  REQUIRE(oracle.is_valid());

  // Input X = 0.0 causes division by zero, returning an uncomputable result.
  const std::vector<value_t> uncomputable_input{0.0};
  const auto res(oracle.tag(uncomputable_input));

  // Missing inference fallback selects class 0 with confidence 3/4 = 0.75.
  CHECK(res.label == 0);
  CHECK(res.sureness == doctest::Approx(3.0 / 4.0));
}

TEST_CASE_FIXTURE(fixture, "binary_oracle")

{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender);
  CHECK(pr.data.selected().read(is) == debug::GENDER_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // BINARY LAMBDA TEAM OF ONE INDIVIDUAL.
  test_team_of_one<src::binary_oracle>(pr);

  // BINARY LAMBDA TEAM OF RANDOM INDIVIDUALS.
  test_team<src::binary_oracle>(pr);
}

TEST_CASE_FIXTURE(fixture, "binary_oracle serialization")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender);
  CHECK(pr.data.selected().read(is) == debug::GENDER_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  // BINARY_LAMBDA_F SERIALIZATION - INDIVIDUAL.
  test_serialization<src::binary_oracle, gp::individual>(pr);

  // BINARY_LAMBDA_F SERIALIZATION - TEAM.
  test_serialization<src::binary_oracle, gp::team<gp::individual>>(pr);
}

TEST_CASE("class_oracle_adaptor")
{
  using namespace ultra;

  struct predictor
  {
    src::classification_result tag(const std::vector<value_t> &) const
    {
      return {1, 0.75};
    }
  };

  const src::class_oracle_adaptor named(predictor{},
                                       {"first class", "second class"});
  const src::class_oracle_adaptor unnamed(predictor{});

  for (const src::basic_class_oracle *oracle : {&named, &unnamed})
  {
    CHECK(oracle->is_valid());
    CHECK((*oracle)({}) == value_t(D_INT(1)));
    CHECK(oracle->tag({}).label == 1);
    CHECK(oracle->tag({}).sureness == doctest::Approx(0.75));
    CHECK(oracle->name(value_t(D_INT(-1))) == "-1");
    CHECK(oracle->name(value_t(D_INT(2))) == "2");
  }

  CHECK(named.name(value_t(D_INT(0))) == "first class");
  CHECK(named.name(value_t(D_INT(1))) == "second class");
  CHECK(named.name(value_t(0.0)) == "first class");
  CHECK(unnamed.name(value_t(D_INT(1))) == "1");

  std::ostringstream saved;
  CHECK_THROWS_AS(src::serialize::save(saved, named), std::logic_error);
}

TEST_CASE_TEMPLATE("Classification oracle names", P,
                   ultra::gp::individual,
                   ultra::gp::team<ultra::gp::individual>)
{
  using namespace ultra;

  fixture f;
  auto &pr(f.pr);
  auto &data(pr.data.selected());
  std::istringstream input(R"(
    CLASS, X
    negative class, -2.0
    negative class, -1.0
    positive class, 1.0
    positive class, 2.0
  )");
  REQUIRE(data.read(input) == 4);
  pr.setup_symbols();

  const auto *x(static_cast<const src::variable *>(pr.sset.decode("X")));
  REQUIRE(x);
  const function *add(pr.insert<real::add>());
  const gp::individual ind({{add, {x, x}}});
  const P prg([&]
  {
    if constexpr (gp::Team<P>)
      return P{{ind, ind}};
    else
      return ind;
  }());

  const auto check_names([&](const auto &oracle, bool named)
  {
    std::stringstream saved;
    REQUIRE(src::serialize::save(saved, oracle));
    const auto loaded(src::serialize::oracle::load<P>(saved, pr.sset));
    REQUIRE(loaded);
    CHECK(loaded->is_valid());

    for (src::class_t i(0); i < data.classes(); ++i)
    {
      const value_t id(static_cast<D_INT>(i));
      const auto expected(named ? data.class_name(i) : std::to_string(i));
      CHECK(oracle.name(id) == expected);
      CHECK(loaded->name(id) == expected);

      const auto text(saved.str());
      const auto pos(text.find(data.class_name(i)));
      if (named)
      {
        REQUIRE(pos != std::string::npos);
        CHECK(text.find(data.class_name(i), pos + data.class_name(i).size())
              == std::string::npos);
      }
      else
        CHECK(pos == std::string::npos);
    }

    for (const auto &example : data)
    {
      const auto expected(oracle.tag(example.input));
      const auto actual(loaded->tag(example.input));
      CHECK(actual.label == expected.label);
      CHECK(actual.sureness == doctest::Approx(expected.sureness));
    }

    for (const D_INT id : {-1, static_cast<D_INT>(data.classes())})
    {
      const auto expected(std::to_string(id));
      CHECK(oracle.name(value_t(id)) == expected);
      CHECK(loaded->name(value_t(id)) == expected);
    }

    auto truncated(saved.str());
    const auto tail_size(named ? data.class_name(data.classes() - 1).size() + 1
                               : 2);
    truncated.erase(truncated.size() - tail_size);
    std::istringstream incomplete(truncated);
    CHECK_THROWS_AS(
      static_cast<void>(src::serialize::oracle::load<P>(incomplete, pr.sset)),
      exception::data_format);
  });

  SUBCASE("Gaussian")
  {
    SUBCASE("Named")
      check_names(src::gaussian_oracle<P>(prg, data), true);
    SUBCASE("Unnamed")
      check_names(src::basic_gaussian_oracle<P, true>(prg, data), false);
    SUBCASE("Borrowed")
      check_names(src::basic_gaussian_oracle<P, false>(prg, data), false);
  }

  SUBCASE("Binary")
  {
    SUBCASE("Named")
      check_names(src::binary_oracle<P>(prg, data), true);
    SUBCASE("Unnamed")
      check_names(src::basic_binary_oracle<P, true>(prg, data), false);
    SUBCASE("Borrowed")
      check_names(src::basic_binary_oracle<P, false>(prg, data), false);
  }

  if constexpr (gp::Team<P>)
    SUBCASE("Team composition")
    {
      using member = src::basic_binary_oracle<gp::individual, true>;
      const gp::individual minority({{add, {10.0, 10.0}}});
      const gp::team<gp::individual> ensemble{{ind, ind, minority}};
      const src::team_class_oracle<member, src::team_composition::mv>
        majority(ensemble, data);
      const src::team_class_oracle<member> winner(ensemble, data);

      const std::vector<value_t> example{-1.0};
      CHECK(majority.tag(example).label == 0);
      CHECK(majority.tag(example).sureness == doctest::Approx(2.0 / 3.0));
      CHECK(winner.tag(example).label == 1);
      CHECK(winner.tag(example).sureness
            == doctest::Approx(member(minority, data).tag(example).sureness));
    }
}

TEST_CASE_FIXTURE(fixture, "Perfect binary_oracle")
{
  using namespace ultra;
  log::reporting_level = log::lWARNING;

  std::istringstream is(debug::gender_trick);
  CHECK(pr.data.selected().read(is) == debug::GENDER_TRICK_COUNT);
  pr.setup_symbols();
  CHECK(pr.sset.enough_terminals());

  const auto *easy(static_cast<const src::variable *>(
                     pr.sset.decode("EASY")));
  CHECK(easy);

  const function *f_add(pr.insert<ultra::real::add>());

  const gp::individual delphi(
    {
      {f_add, {easy, easy}}
    });

  const src::binary_oracle oracle(delphi, pr.data.selected());
  for (const auto &e : pr.data.selected())
    CHECK(oracle.tag(e.input).label == label(e));
}

TEST_CASE("Parallel oracles")
{
  using namespace ultra;

  src::dataframe df;

  df.set_schema({{"Y", d_int},
                 {"X1", d_int}, {"X2", d_int}, {"X3", d_int}, {"X4", d_int}});

  for (std::size_t i(0); i < 1000; ++i)
  {
    src::example ex;

    ex.input = {random::sup<D_INT>(1000), random::sup<D_INT>(1000),
                random::sup<D_INT>(1000), random::sup<D_INT>(1000)};
    ex.output = random::sup<D_INT>(1000);

    df.push_back(ex);
  }

  src::problem prob(df);
  prob.params.init();

  prob.insert<integer::add>();
  prob.insert<integer::sub>();
  prob.insert<integer::div>();
  prob.insert<integer::mul>();

  std::vector<gp::individual> individuals;
  for (std::size_t i(0); i < 200; ++i)
    individuals.emplace_back(prob);

  const auto standard_sum([](const ultra::src::dataframe &d,
                             const ultra::gp::individual &ind)
  {
    src::reg_oracle oracle(ind);

    int sum(0.0);
    for (const auto &in : d)
      if (const auto v(oracle(in.input)); has_value(v))
        sum += std::get<D_INT>(v);

    return sum;
  });

  const auto par_reduce_sum([](const ultra::src::dataframe &d,
                               const ultra::gp::individual &ind)
  {
    const auto stride(
      std::max<std::size_t>(1, std::thread::hardware_concurrency()));

    std::vector<std::future<int>> futures;
    futures.reserve(stride);

    for (std::size_t i(0); i < stride; ++i)
      futures.emplace_back(
        std::async(
          std::launch::async,
          [&ind, &d, stride](std::size_t start)
          {
            // Thread-local oracle.
            src::reg_oracle oracle(ind);

            const auto end(d.end());
            auto it(std::ranges::next(d.begin(), start, end));

            int ps(0);

            if (it == end)
              return ps;

            if (const auto v(oracle(it->input)); has_value(v))
              ps = std::get<D_INT>(v);

            std::ranges::advance(it, stride, end);

            while (it != end)
            {
              if (const auto v(oracle(it->input)); has_value(v))
                ps += std::get<D_INT>(v);

              std::ranges::advance(it, stride, end);
            }

            return ps;
          }, i));

    // Reduction.
    int sum(0);

    for (auto &f : futures)
    {
      const auto ps(f.get());
      sum += ps;
    }

    return sum;
  });

  for (const auto &ind : individuals)
    CHECK(standard_sum(df, ind) == par_reduce_sum(df, ind));
}

}  // TEST_SUITE("ORACLE")
