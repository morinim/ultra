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

#include "kernel/refiner.h"
#include "kernel/de/numerical_refiner.h"
#include "kernel/gp/individual.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <cmath>
#include <vector>

namespace
{

[[nodiscard]] bool same_structure_ignoring_numeric(
  const ultra::gp::individual &lhs, const ultra::gp::individual &rhs)
{
  using namespace ultra;

  if (lhs.size() != rhs.size())
    return false;

  if (lhs.categories() != rhs.categories())
    return false;

  for (locus::index_t i(0); i < lhs.size(); ++i)
    for (symbol::category_t c(0); c < lhs.categories(); ++c)
    {
      const locus loc{i, c};
      const auto &gl(lhs[loc]);
      const auto &gr(rhs[loc]);

      if (gl.category() != gr.category())
        return false;

      if (gl.func != gr.func)
        return false;

      if (gl.args.size() != gr.args.size())
        return false;

      for (std::size_t a(0); a < gl.args.size(); ++a)
      {
        const bool lhs_num(
          std::holds_alternative<ultra::D_DOUBLE>(gl.args[a])
          || std::holds_alternative<ultra::D_INT>(gl.args[a]));
        const bool rhs_num(
          std::holds_alternative<ultra::D_DOUBLE>(gr.args[a])
          || std::holds_alternative<ultra::D_INT>(gr.args[a]));

        if (lhs_num != rhs_num)
          return false;
        if (lhs_num)
          continue;

        if (gl.args[a] != gr.args[a])
          return false;
      }
    }

  return true;
}

}  // namespace


TEST_SUITE("de::numerical_refiner")
{

TEST_CASE_FIXTURE(fixture1, "No optimisable slots is a no-op")
{
  using namespace ultra;

  gp::individual ind(
    {
      {f_add, {z, z}},              // [0] ADD Z() Z()
      {f_sub, {0_addr, z}}          // [1] SUB [0] Z()
    });

  const auto original(ind);

  const refiner opt(prob);

  const auto eva([](const gp::individual &) { return 0.0; });

  de::optimise(opt, ind, eva);

  CHECK(ind == original);
  CHECK(ind.signature() == original.signature());
}

TEST_CASE_FIXTURE(fixture1, "Optimisation preserves validity and structure")
{
  using namespace ultra;

  gp::individual ind(
    {
      {f_add, {3.0, 2.0}},         // [0]
      {f_add, {0_addr, 1}},        // [1]
      {f_sub, {1_addr, 0_addr}}    // [2]
    });

  const auto original(ind);
  const auto before_dv(extract_decision_vector(ind));

  REQUIRE(before_dv.is_valid());
  REQUIRE(before_dv.size() == 3);

  std::vector<double> target(before_dv.values);
  target[0] += 0.75;
  target[1] -= 0.50;
  target[2] += 0.25;

  const auto eva([target](const gp::individual &cand)
  {
    const auto dv(extract_decision_vector(cand));

    double s(0.0);
    for (std::size_t i(0); i < dv.size(); ++i)
    {
      const auto d(dv.values[i] - target[i]);
      s -= d * d;
    }

    return s;
  });

  prob.params.refinement.de.individuals = 40;
  prob.params.refinement.de.generations = 60;

  const refiner opt(prob);
  de::optimise(opt, ind, eva);

  CHECK(ind.is_valid());
  CHECK(same_structure_ignoring_numeric(ind, original));

  const auto after_dv(extract_decision_vector(ind));
  REQUIRE(after_dv.is_valid());
  REQUIRE(after_dv.size() == before_dv.size());

  for (std::size_t i(0); i < before_dv.size(); ++i)
  {
    CHECK(after_dv.coords[i].coord.loc == before_dv.coords[i].coord.loc);
    CHECK(after_dv.coords[i].coord.arg_index
          == before_dv.coords[i].coord.arg_index);
    CHECK(after_dv.coords[i].kind == before_dv.coords[i].kind);
  }
}

TEST_CASE_FIXTURE(fixture1,
                  "Optimisation improves a simple real-valued objective")
{
  using namespace ultra;

  gp::individual ind(
    {
      {f_add, {3.0, 2.0}},         // [0]
      {f_add, {0_addr, 1.0}},      // [1]
      {f_sub, {1_addr, 0_addr}}    // [2]
    });

  const auto base_dv(extract_decision_vector(ind));
  REQUIRE(base_dv.is_valid());
  REQUIRE(base_dv.size() == 3);

  std::vector<double> target(base_dv.values);
  target[0] += 0.50;
  target[1] -= 0.75;
  target[2] += 0.25;

  const auto eva([target](const gp::individual &cand)
  {
    const auto dv(extract_decision_vector(cand));

    double s(0.0);
    for (std::size_t i(0); i < dv.size(); ++i)
    {
      const auto d(dv.values[i] - target[i]);
      s -= d * d;
    }

    return s;
  });

  const auto before(eva(ind));

  prob.params.refinement.de.individuals = 40;
  prob.params.refinement.de.generations = 60;

  const refiner opt(prob);
  de::optimise(opt, ind, eva);

  const auto after(eva(ind));

  CHECK(ind.is_valid());
  CHECK(after >= before);
}

TEST_CASE_FIXTURE(fixture1,
                  "Optimisation writes integer slots back as integers")
{
  using namespace ultra;

  gp::individual ind(
    {
      {f_add, {z, 1}},             // [0]
      {f_sub, {0_addr, z}}         // [1]
    });

  const auto before(std::get<D_INT>(ind[{0, 0}].args[1]));
  CHECK(before == 1);

  const auto eva([](const gp::individual &cand)
  {
    const auto dv(extract_decision_vector(cand));
    REQUIRE(dv.size() == 1);

    // Optimum near 1.8, so applying the decision vector should round to 2.
    const auto d(dv.values[0] - 1.8);
    return -(d * d);
  });

  prob.params.refinement.de.individuals = 40;
  prob.params.refinement.de.generations = 80;

  const refiner opt(prob);
  de::optimise(opt, ind, eva);

  CHECK(ind.is_valid());
  CHECK(std::holds_alternative<D_INT>(ind[{0, 0}].args[1]));
  CHECK(std::get<D_INT>(ind[{0, 0}].args[1]) == 2);
}

}  // TEST_SUITE
