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

#include "kernel/evolution_recombination.h"
#include "kernel/de/individual.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"
#include "test/fixture4.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <concepts>
#include <cstdlib>
#include <iostream>

namespace ultra
{

class base_testing_individual : public individual
{
public:
  [[nodiscard]] bool empty() const noexcept { return false; }

  unsigned mutations {0};

private:
  [[nodiscard]] hash_t hash() const override { return signature_; }

  [[nodiscard]] bool load_impl(std::istream &, const symbol_set &) override
  {
    return false;
  }
  [[nodiscard]] bool save_impl(std::ostream &) const override { return false; }
  void print_impl(std::ostream &, out::print_format_t) const override {}
};

class colliding_individual final : public base_testing_individual
{
public:
  explicit colliding_individual(std::uint64_t signature = 1)
  {
    signature_ = hash_t(signature);
  }

  void mutation(const problem &, double = 1.0)
  {
    ++mutations;
    signature_ = hash_t(mutations + 1);
  }
};

class mutation_only_individual final : public base_testing_individual
{
public:
  explicit mutation_only_individual(unsigned p = 0) : parent(p) {}

  void mutation(const problem &, double = 1.0) { ++mutations; }

  unsigned parent;
};

template<std::derived_from<base_testing_individual> I>
[[nodiscard]] I crossover(const problem &, const I &parent, const I &)
{
  return parent;
}

}  // namespace ultra

struct increasing_evaluator
{
  using individual_t = ultra::gp::individual;

  [[nodiscard]] double operator()(const individual_t &ind) const
  {
    evaluated.push_back(ind);
    return static_cast<double>(evaluated.size());
  }

  mutable std::vector<individual_t> evaluated;
};

struct colliding_evaluator
{
  [[nodiscard]] double operator()(const ultra::colliding_individual &) const
  {
    return 0.0;
  }
};

struct mutation_only_evaluator
{
  [[nodiscard]] double operator()(
    const ultra::mutation_only_individual &) const
  {
    return 0.0;
  }
};

TEST_SUITE("EVOLUTION RECOMBINATION")
{

TEST_CASE_FIXTURE(fixture1, "Base")
{
  using namespace ultra;

  test_evaluator<gp::individual> eva(test_evaluator_type::realistic);
  recombination::base recombine(eva, prob);

  std::vector parents = { gp::individual(prob), gp::individual(prob) };
  while (parents[0] == parents[1])
    parents[1] = gp::individual(prob);

  SUBCASE("No crossover and no mutation")
  {
    prob.params.evolution.p_cross = 0.0;
    prob.params.evolution.p_mutation = 0.0;

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(parents));

      CHECK(off.is_valid());
      const bool one_or_the_other(off == parents[0] || off == parents[1]);
      CHECK(one_or_the_other);
    }
  }

  SUBCASE("No mutation")
  {
    prob.params.evolution.p_cross = 1.0;
    prob.params.evolution.p_mutation = 0.0;

    std::vector same_parents = {parents[0], parents[0]};

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(recombine(same_parents));

      CHECK(off.is_valid());
      CHECK(off == same_parents[0]);
    }
  }

  // The test disables crossover, forces mutation, then generates multiple
  // offspring. It verifies each offspring is mutated exactly once and confirms
  // both parents are selected as mutation sources.
  SUBCASE("Mutation only")
  {
    prob.params.evolution.p_cross = 0.0;
    prob.params.evolution.p_mutation = 1.0;

    mutation_only_evaluator mutation_only_eva;
    recombination::base mutation_only_recombine(mutation_only_eva, prob);
    const std::vector mutation_only_parents =
    {
      mutation_only_individual(1), mutation_only_individual(2)
    };
    bool selected_first(false);
    bool selected_second(false);

    for (unsigned i(0); i < 100; ++i)
    {
      const auto off(mutation_only_recombine(mutation_only_parents));

      CHECK(off.mutations == 1);
      selected_first |= off.parent == 1;
      selected_second |= off.parent == 2;
    }

    CHECK(selected_first);
    CHECK(selected_second);
  }

  SUBCASE("Standard")
  {
    constexpr unsigned N(200);
    unsigned distinct(0);

    for (unsigned repetitions(N); repetitions; --repetitions)
    {
      const auto off(recombine(parents));
      CHECK(off.is_valid());

      if (off != parents[0] && off != parents[1])
        ++distinct;
    }

    CHECK(static_cast<double>(distinct) / N
          > prob.params.evolution.p_cross - 0.1);
  }

  SUBCASE("Brood recombination")
  {
    constexpr unsigned brood_size(5);

    prob.params.evolution.p_cross = 1.0;
    prob.params.evolution.p_mutation = 0.0;
    prob.params.evolution.brood_recombination = brood_size;

    increasing_evaluator increasing_eva;
    recombination::base brood_recombine(increasing_eva, prob);

    const auto off(brood_recombine(parents));

    REQUIRE(increasing_eva.evaluated.size() == brood_size);
    CHECK(off == increasing_eva.evaluated.back());
  }

  SUBCASE("Mutation avoids both parent signatures")
  {
    prob.params.evolution.p_cross = 1.0;
    prob.params.evolution.p_mutation = 1.0;

    colliding_evaluator colliding_eva;
    recombination::base colliding_recombine(colliding_eva, prob);

    // Parent signatures are 1 and 2.
    const std::vector colliding_parents =
    {
      colliding_individual(1), colliding_individual(2)
    };

    // - Crossover returns the first parent, so the offspring initially has
    //   signature 1.
    // - The first mutation changes its signature to 2, which still matches the
    //   second parent.
    // - The second mutation changes it to 3, which matches neither parent.
    const auto off(colliding_recombine(colliding_parents));

    // The assertions directly exercises the while loop that prevents offspring
    // retaining either parent's signature.
    CHECK(off.mutations == 2);
    CHECK(off.signature() != colliding_parents[0].signature());
    CHECK(off.signature() != colliding_parents[1].signature());
  }
}

TEST_CASE_FIXTURE(fixture4, "DE")
{
  using namespace ultra;

  recombination::de recombine(prob);

  SUBCASE("Zero p_cross")
  {
    prob.params.evolution.p_cross = 0;

    for (unsigned iterations(100); iterations; --iterations)
    {
      std::vector<de::individual> pop =
      {
        de::individual(prob), de::individual(prob),
        de::individual(prob), de::individual(prob)
      };

      const selection::de::selected_refs<de::individual> parents
        {pop[0], pop[1], pop[2], pop[3]};

      const auto x(recombine(parents));

      const auto last(x.parameters() - 1);
      for (std::size_t i(0); i < last; ++i)
        CHECK(parents.target[i] == doctest::Approx(x[i]));

      CHECK(parents.target[last] != doctest::Approx(x[last]));
    }
  }

  SUBCASE("No diether")
  {
    prob.params.de.weight = {std::nextafter(1.0, 0.0), 1.0};

    for (unsigned iterations(100); iterations; --iterations)
    {
      std::vector pop =
      {
        de::individual(prob), de::individual(prob),
        de::individual(prob), de::individual(prob)
      };
      const selection::de::selected_refs<de::individual> parents
        {pop[0], pop[1], pop[2], pop[3]};

      const auto x(recombine(parents));

      const auto last(x.parameters() - 1);
      for (std::size_t i(0); i < last; ++i)
      {
        const bool no_cross(parents.target[i] == doctest::Approx(x[i]));
        const bool cross(parents.base[i] + parents.a[i] - parents.b[i]
                         == doctest::Approx(x[i]));
        const bool valid(no_cross || cross);
        CHECK(valid);
      }

      CHECK(parents.base[last] + parents.a[last] - parents.b[last]
            == doctest::Approx(x[last]));
    }
  }
}

}  // TEST_SUITE
